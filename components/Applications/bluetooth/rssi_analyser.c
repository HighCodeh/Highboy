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

#include "rssi_analyser.h"
#include "st7789.h"
#include "pin_def.h"
#include "driver/gpio.h"
#include "bluetooth_scanner.h"
#include "esp_log.h"
#include "esp_timer.h" // ADICIONADO: Para controle de tempo preciso
#include <string.h>

// --- DEFINIÇÕES DE CORES E LAYOUT ---
#define COLOR_BACKGROUND         ST7789_COLOR_BLACK
#define COLOR_TEXT_PRIMARY       ST7789_COLOR_WHITE
#define COLOR_TEXT_SECONDARY     ST7789_COLOR_GRAY
#define COLOR_GRID               0x31A6 // Cinzento escuro para a grelha
#define COLOR_HIGHLIGHT          ST7789_COLOR_PURPLE
#define COLOR_DIVIDER            0x4228 // Cinzento para linhas divisórias

// --- DEFINIÇÕES PARA O GRÁFICO RSSI ---
#define GRAPH_X                  10
#define GRAPH_Y                  50
#define GRAPH_WIDTH              220
#define GRAPH_HEIGHT             150
#define NUM_SAMPLES              22

// --- CORES PARA A CURVA RSSI ---
static const uint16_t GRAPH_COLOR = 0x07E0; // Verde lima

// --- Variáveis estáticas para o monitoramento ---
static ble_addr_t s_monitored_device_addr;
static volatile int8_t s_latest_rssi_value;
static const char *TAG = "RSSI_ANALYSER";

// --- Funções auxiliares estáticas ---
static int map_value(int value, int in_min, int in_max, int out_min, int out_max) {
    if (in_max == in_min) return out_min;
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static void draw_rssi_graph_background(void) {
    int y_base = GRAPH_Y + GRAPH_HEIGHT;
    for (int rssi = -20; rssi >= -100; rssi -= 20) {
        int y_pos = map_value(rssi, -100, 0, y_base, GRAPH_Y);
        st7789_draw_hline_fb(GRAPH_X, y_pos, GRAPH_WIDTH, COLOR_GRID);
        char rssi_str[5];
        snprintf(rssi_str, sizeof(rssi_str), "%d", rssi);
        st7789_draw_text_fb(GRAPH_X - 25, y_pos - 4, rssi_str, COLOR_TEXT_SECONDARY, COLOR_BACKGROUND);
    }
    st7789_draw_rect_fb(GRAPH_X, GRAPH_Y, GRAPH_WIDTH, GRAPH_HEIGHT, COLOR_TEXT_SECONDARY);
}

static void draw_rssi_curve(const int* rssi_history, int num_points) {
    int y_base = GRAPH_Y + GRAPH_HEIGHT;
    int prev_x = -1, prev_y = -1;
    for (int i = 0; i < num_points; i++) {
        int current_x = map_value(i, 0, NUM_SAMPLES - 1, GRAPH_X, GRAPH_X + GRAPH_WIDTH - 1);
        int y_curve = map_value(rssi_history[i], -100, 0, y_base, GRAPH_Y);
        if (y_curve < GRAPH_Y) y_curve = GRAPH_Y;
        if (y_curve > y_base) y_curve = y_base;
        if (prev_x != -1) {
            st7789_draw_line_fb(prev_x, prev_y, current_x, y_curve, GRAPH_COLOR);
        }
        prev_x = current_x;
        prev_y = y_curve;
    }
}

// --- Callback para o scanner ---
static void rssi_update_callback(const discovered_device_t *device) {
    if (ble_addr_cmp(&device->addr, &s_monitored_device_addr) == 0) {
        s_latest_rssi_value = device->rssi;
    }
}

// --- Função Pública Principal ---
void show_rssi_analyser(const BtDevice *dev) {
    if (!dev) {
        ESP_LOGE(TAG, "show_rssi_analyser chamado com ponteiro nulo.");
        return;
    }

    // Inicializa o scanner de RSSI
    s_monitored_device_addr = dev->addr;
    s_latest_rssi_value = dev->rssi;
    if (bluetooth_scanner_start_rssi_monitor(rssi_update_callback) != ESP_OK) {
        st7789_fill_screen_fb(COLOR_BACKGROUND);
        st7789_draw_text_fb(10, 100, "Erro ao iniciar monitor!", COLOR_TEXT_SECONDARY, COLOR_BACKGROUND);
        st7789_flush();
        vTaskDelay(pdMS_TO_TICKS(2000));
        return;
    }

    int rssi_history[NUM_SAMPLES];
    for (int i = 0; i < NUM_SAMPLES; i++) {
        rssi_history[i] = -100;
    }
    int current_index = 0;
    
    // Variáveis de controle do loop
    bool monitoring = true;
    bool needs_redraw = true; // Força o desenho da tela na primeira iteração
    uint64_t last_update_time = 0;
    const uint64_t update_interval_us = 500 * 1000; // 500ms em microssegundos

    while (monitoring) {
        // --- 1. VERIFICAÇÃO DE INPUT (muito responsivo) ---
        // Esta parte do código executa a cada ~50ms
        if (!gpio_get_level(BTN_BACK)) {
            while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(20)); // Debounce
            monitoring = false; // Sinaliza para sair do loop
            continue; // Pula para a próxima iteração para sair imediatamente
        }

        // --- 2. LÓGICA DE ATUALIZAÇÃO DA TELA (baseada no tempo) ---
        uint64_t current_time = esp_timer_get_time();
        if ((current_time - last_update_time >= update_interval_us) || needs_redraw) {
            
            // Atualiza os dados do histórico
            last_update_time = current_time;
            int current_rssi = s_latest_rssi_value;
            rssi_history[current_index] = current_rssi;
            current_index = (current_index + 1) % NUM_SAMPLES;

            // Prepara buffer ordenado para o desenho
            int ordered_rssi[NUM_SAMPLES];
            for(int i = 0; i < NUM_SAMPLES; i++) {
                ordered_rssi[i] = rssi_history[(current_index + i) % NUM_SAMPLES];
            }

            // Inicia o redesenho da tela
            st7789_fill_screen_fb(COLOR_BACKGROUND);

            // Título
            char title[32];
            snprintf(title, sizeof(title), "RSSI: %.20s", dev->name);
            st7789_set_text_size(2);
            st7789_draw_text_fb(10, 10, title, COLOR_TEXT_PRIMARY, COLOR_BACKGROUND);
            st7789_set_text_size(1);
            st7789_draw_hline_fb(10, 35, 220, COLOR_DIVIDER);

            // Fundo do gráfico e curva
            draw_rssi_graph_background();
            draw_rssi_curve(ordered_rssi, NUM_SAMPLES);
            
            // Textos informativos
            char rssi_str[16];
            snprintf(rssi_str, sizeof(rssi_str), "Atual: %d dBm", current_rssi);
            st7789_draw_text_fb(10, 210, rssi_str, COLOR_HIGHLIGHT, COLOR_BACKGROUND);
            st7789_draw_text_fb(10, 225, "BACK: Voltar", COLOR_TEXT_SECONDARY, COLOR_BACKGROUND);

            // Envia o frame buffer para o display
            st7789_flush();
            needs_redraw = false; // Marca que a tela foi redesenhada
        }

        // --- 3. PAUSA CURTA ---
        // Pequeno delay para evitar que o loop consuma 100% da CPU,
        // mas curto o suficiente para manter a leitura dos botões responsiva.
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // --- LIMPEZA ---
    bluetooth_scanner_stop();
}
