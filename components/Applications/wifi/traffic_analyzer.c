// Copyright (c) 2025 HIGH CODE LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "traffic_analyzer.h"
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_private/wifi.h"
#include "st7789.h"
#include "pin_def.h"
#include "driver/gpio.h"
#include "storage_write.h"

// --- Definições de Cores e Layout ---
#define ST7789_COLOR_ORANGE     0xFD20

#define GRAPH_X 10
#define GRAPH_Y 60
#define GRAPH_WIDTH 220
#define GRAPH_HEIGHT 100
#define HISTORY_SIZE GRAPH_WIDTH

typedef struct {
    uint32_t magic_number;
    uint16_t version_major;
    uint16_t version_minor;
    int32_t  thiszone;
    uint32_t sigfigs;
    uint32_t snaplen;
    uint32_t network;
} pcap_global_header_t;

typedef struct {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
} pcap_record_header_t;

typedef struct {
    uint32_t len;
    uint8_t data[1500];
} packet_data_t;

static atomic_int g_mgmt_packets = 0, g_ctrl_packets = 0, g_data_packets = 0;
static int g_pps_history[HISTORY_SIZE] = {0};
static int g_history_index = 0, g_current_total_pps = 0, g_current_mgmt_pps = 0;
static int g_current_ctrl_pps = 0, g_current_data_pps = 0, g_peak_pps = 0;
static bool g_is_capturing_to_sd = false;
static TaskHandle_t g_traffic_task_handle = NULL;
static TaskHandle_t g_pcap_writer_task_handle = NULL;
static QueueHandle_t g_pcap_queue = NULL;
static char g_capture_filename[64];
static uint32_t g_packets_captured_count = 0;

static void wifi_sniffer_cb(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    switch (type) {
        case WIFI_PKT_MGMT: atomic_fetch_add(&g_mgmt_packets, 1); break;
        case WIFI_PKT_CTRL: atomic_fetch_add(&g_ctrl_packets, 1); break;
        case WIFI_PKT_DATA: atomic_fetch_add(&g_data_packets, 1); break;
        default: break;
    }
    if (g_is_capturing_to_sd && g_pcap_queue != NULL && pkt->rx_ctrl.sig_len <= sizeof(packet_data_t) - sizeof(uint32_t)) {
        packet_data_t p_data;
        p_data.len = pkt->rx_ctrl.sig_len;
        memcpy(p_data.data, pkt->payload, p_data.len);
        xQueueSendToBack(g_pcap_queue, &p_data, 0);
    }
}

// CORREÇÃO: A tarefa agora é responsável por apagar a sua própria fila
static void pcap_writer_task(void *pvParameters) {
    packet_data_t received_packet;
    pcap_record_header_t record_header;
    while (g_is_capturing_to_sd) {
        if (xQueueReceive(g_pcap_queue, &received_packet, pdMS_TO_TICKS(1000)) == pdPASS) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            record_header.ts_sec = tv.tv_sec;
            record_header.ts_usec = tv.tv_usec;
            record_header.incl_len = received_packet.len;
            record_header.orig_len = received_packet.len;
            size_t total_size = sizeof(pcap_record_header_t) + received_packet.len;
            uint8_t* write_buffer = malloc(total_size);
            if (write_buffer) {
                memcpy(write_buffer, &record_header, sizeof(pcap_record_header_t));
                memcpy(write_buffer + sizeof(pcap_record_header_t), received_packet.data, received_packet.len);
                storage_append_binary(g_capture_filename, write_buffer, total_size);
                free(write_buffer);
                g_packets_captured_count++;
            }
        }
    }
    // --- ROTINA DE LIMPEZA DA TAREFA ---
    if (g_pcap_queue != NULL) {
        vQueueDelete(g_pcap_queue); // 1. A tarefa apaga a fila que usou
        g_pcap_queue = NULL;        // 2. Sinaliza que a fila não existe mais
    }
    g_pcap_writer_task_handle = NULL;
    vTaskDelete(NULL); // 3. A tarefa se autodestrói
}

static void traffic_update_task(void *pvParameters) {
    const int update_interval_ms = 250;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(update_interval_ms));
        int mgmt_count = atomic_exchange(&g_mgmt_packets, 0);
        int ctrl_count = atomic_exchange(&g_ctrl_packets, 0);
        int data_count = atomic_exchange(&g_data_packets, 0);
        int factor = 1000 / update_interval_ms;
        g_current_mgmt_pps = mgmt_count * factor;
        g_current_ctrl_pps = ctrl_count * factor;
        g_current_data_pps = data_count * factor;
        g_current_total_pps = g_current_mgmt_pps + g_current_ctrl_pps + g_current_data_pps;
        if (g_current_total_pps > g_peak_pps) g_peak_pps = g_current_total_pps;
        g_pps_history[g_history_index] = g_current_total_pps;
        g_history_index = (g_history_index + 1) % HISTORY_SIZE;
    }
}

static int map_value(int value, int in_min, int in_max, int out_min, int out_max) {
    if (value <= in_min) return out_min;
    if (value >= in_max) return out_max;
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static void draw_ui_on_framebuffer(int channel) {
    char buffer[64];
    int max_graph_pps = 500;
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    snprintf(buffer, sizeof(buffer), "Analisador | Canal: %d", channel);
    st7789_draw_text_centered(120, 5, buffer, ST7789_COLOR_PURPLE);
    if (g_is_capturing_to_sd) {
        st7789_fill_rect_fb(220, 5, 10, 10, ST7789_COLOR_RED);
        st7789_draw_text_fb(150, 220, g_capture_filename, ST7789_COLOR_GRAY, ST7789_COLOR_BLACK);
        snprintf(buffer, sizeof(buffer), "Pacotes: %lu", g_packets_captured_count);
        st7789_draw_text_fb(10, 220, buffer, ST7789_COLOR_GRAY, ST7789_COLOR_BLACK);
    } else {
         st7789_draw_text_fb(10, 220, "Pressione OK para gravar", ST7789_COLOR_GRAY, ST7789_COLOR_BLACK);
    }
    st7789_draw_rect_fb(GRAPH_X, GRAPH_Y, GRAPH_WIDTH, GRAPH_HEIGHT, ST7789_COLOR_DARKGRAY);
    st7789_draw_text_fb(GRAPH_X, 170, "Gestao:", ST7789_COLOR_CYAN, ST7789_COLOR_BLACK);
    st7789_draw_text_fb(GRAPH_X, 185, "Controlo:", ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK);
    st7789_draw_text_fb(GRAPH_X, 200, "Dados:", ST7789_COLOR_GREEN, ST7789_COLOR_BLACK);
    for (int i = 0; i < GRAPH_WIDTH - 1; i++) {
        int hist_idx1 = (g_history_index + i) % HISTORY_SIZE;
        int hist_idx2 = (g_history_index + i + 1) % HISTORY_SIZE;
        int y1 = GRAPH_Y + GRAPH_HEIGHT - map_value(g_pps_history[hist_idx1], 0, max_graph_pps, 0, GRAPH_HEIGHT -1);
        int y2 = GRAPH_Y + GRAPH_HEIGHT - map_value(g_pps_history[hist_idx2], 0, max_graph_pps, 0, GRAPH_HEIGHT -1);
        st7789_draw_line_fb(GRAPH_X + i, y1, GRAPH_X + i + 1, y2, ST7789_COLOR_RED);
    }
    snprintf(buffer, sizeof(buffer), "PPS: %d", g_current_total_pps);
    st7789_draw_text_fb(15, 35, buffer, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    snprintf(buffer, sizeof(buffer), "Pico: %d", g_peak_pps);
    st7789_draw_text_fb(140, 35, buffer, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    int total_for_bar = (g_current_total_pps == 0) ? 1 : g_current_total_pps;
    st7789_fill_rect_fb(80, 170, map_value(g_current_mgmt_pps, 0, total_for_bar, 0, 100), 10, ST7789_COLOR_CYAN);
    st7789_fill_rect_fb(80, 185, map_value(g_current_ctrl_pps, 0, total_for_bar, 0, 100), 10, ST7789_COLOR_YELLOW);
    st7789_fill_rect_fb(80, 200, map_value(g_current_data_pps, 0, total_for_bar, 0, 100), 10, ST7789_COLOR_GREEN);
    snprintf(buffer, sizeof(buffer), "%d", g_current_mgmt_pps); st7789_draw_text_fb(190, 170, buffer, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    snprintf(buffer, sizeof(buffer), "%d", g_current_ctrl_pps); st7789_draw_text_fb(190, 185, buffer, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    snprintf(buffer, sizeof(buffer), "%d", g_current_data_pps); st7789_draw_text_fb(190, 200, buffer, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
}

void show_traffic_analyzer(void) {
    int current_channel = 1;
    bool running = true;
    g_history_index = 0;
    g_peak_pps = 0;
    memset(g_pps_history, 0, sizeof(g_pps_history));
    g_is_capturing_to_sd = false;
    xTaskCreate(traffic_update_task, "traffic_task", 2048, NULL, 5, &g_traffic_task_handle);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_cb);
    while (running) {
        if (!gpio_get_level(BTN_UP)) {
            while(!gpio_get_level(BTN_UP)) vTaskDelay(pdMS_TO_TICKS(10));
            current_channel = (current_channel % 13) + 1;
            esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
            g_peak_pps = 0;
        } else if (!gpio_get_level(BTN_DOWN)) {
            while(!gpio_get_level(BTN_DOWN)) vTaskDelay(pdMS_TO_TICKS(10));
            current_channel = (current_channel == 1) ? 13 : current_channel - 1;
            esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
            g_peak_pps = 0;
        } else if (!gpio_get_level(BTN_BACK)) {
            while(!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(10));
            running = false;
        } else if (!gpio_get_level(BTN_OK)) {
            while(!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(10));
            g_is_capturing_to_sd = !g_is_capturing_to_sd;
            if (g_is_capturing_to_sd) {
                time_t now;
                struct tm timeinfo;
                time(&now);
                localtime_r(&now, &timeinfo);
                strftime(g_capture_filename, sizeof(g_capture_filename), "/sdcard/captura_%Y%m%d_%H%M%S.pcap", &timeinfo);
                g_packets_captured_count = 0;
                pcap_global_header_t global_header = {
                    .magic_number = 0xa1b2c3d4, .version_major = 2, .version_minor = 4,
                    .thiszone = 0, .sigfigs = 0, .snaplen = 65535, .network = 105
                };
                storage_write_binary(g_capture_filename, &global_header, sizeof(global_header));
                g_pcap_queue = xQueueCreate(20, sizeof(packet_data_t));
                if (g_pcap_queue == NULL) {
                    g_is_capturing_to_sd = false;
                    st7789_fill_rect_fb(0, 80, 240, 80, ST7789_COLOR_RED);
                    st7789_draw_text_centered(120, 100, "Erro: Pouca Memoria!", ST7789_COLOR_WHITE);
                    st7789_flush();
                    vTaskDelay(pdMS_TO_TICKS(2000));
                } else {
                    xTaskCreate(pcap_writer_task, "pcap_writer", 4096, NULL, 4, &g_pcap_writer_task_handle);
                }
            } else {
                // CORREÇÃO: A UI agora apenas sinaliza para a tarefa parar.
                // A tarefa de escrita tratará de apagar a fila.
                // Não fazemos mais nada aqui.
            }
        }
        draw_ui_on_framebuffer(current_channel);
        st7789_flush();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (g_is_capturing_to_sd) {
        g_is_capturing_to_sd = false;
        vTaskDelay(pdMS_TO_TICKS(1100)); // Espera um pouco mais para a tarefa de escrita parar e limpar
    }
    vTaskDelete(g_traffic_task_handle);
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
}
