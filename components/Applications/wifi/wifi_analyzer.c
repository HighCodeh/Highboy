/*
 * -----------------------------------------------------------------------------
 * Ficheiro: wifi_analyzer.c
 * Descrição: Implementação de um analisador de redes Wi-Fi com interface
 * gráfica profissional e dashboard de detalhes. (Versão Corrigida)
 * Autor: Parceiro de Programacao
 * -----------------------------------------------------------------------------
 */

 #include "wifi_analyzer.h"
 #include "st7789.h"
#include "wifi_service.h"
 #include "wifi_deauther.h"
 #include "pin_def.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "esp_wifi_types.h"
 #include "driver/gpio.h"
 #include "string.h"
 #include "stdio.h"
 #include "icons.h"
 #include <math.h>
 #include <stdlib.h> // Necessário para a função qsort
 
 // --- DEFINIÇÕES DE CORES E LAYOUT ---
 #define COLOR_BACKGROUND          ST7789_COLOR_BLACK
 #define COLOR_TEXT_PRIMARY        ST7789_COLOR_WHITE
 #define COLOR_TEXT_SECONDARY      ST7789_COLOR_GRAY
 #define COLOR_GRID                0x31A6 // Um cinzento escuro para a grelha
 #define COLOR_HIGHLIGHT           ST7789_COLOR_PURPLE
 #define COLOR_SELECTED_OUTLINE    ST7789_COLOR_YELLOW
 #define COLOR_DIVIDER             0x4228 // Cinzento para linhas divisórias
 #define ST7789_COLOR_LIME         0x07E0 // ✨ CORRIGIDO: Adicionada a definição da cor que faltava
 
 // --- ✨ CORRIGIDO: Readicionadas as definições de layout que faltavam ---
 #define GRAPH_X                   20
 #define GRAPH_Y                   40
 #define GRAPH_WIDTH               200
 #define GRAPH_HEIGHT              120
 #define DETAILS_BOX_Y             (GRAPH_Y + GRAPH_HEIGHT + 15)
 #define DETAILS_BOX_HEIGHT        60
 // --- FIM DA CORREÇÃO ---
 
 // --- CORES PARA AS CURVAS DAS REDES ---
 static const uint16_t GRAPH_COLORS[] = {
     0xF800, 0x05E0, 0x041F, 0xFFE0, 0xF81F, 0x07FF, 0xFD20, 0xAFE5,
 };
 #define NUM_GRAPH_COLORS (sizeof(GRAPH_COLORS) / sizeof(uint16_t))
 
 // --- PROTÓTIPOS DE FUNÇÕES ESTÁTICAS ---
 static int compare_ap_records(const void *a, const void *b);
 static int map_value(int value, int in_min, int in_max, int out_min, int out_max);
 static void draw_channel_graph_background(void);
 static void draw_network_curve(const wifi_ap_record_t *ap, uint16_t color, bool is_selected);
 static void draw_detailed_info_box(const wifi_ap_record_t *ap);
 static const char* get_auth_mode_string(wifi_auth_mode_t authmode);
 static void show_detailed_network_view(const wifi_ap_record_t *ap);
 static const char* get_cipher_string(wifi_cipher_type_t cipher);
 static void draw_signal_bars(int x, int y, int rssi);
 static void draw_lock_icon(int x, int y, uint16_t color);
 static int draw_standard_tag(int x, int y, const char* label, bool is_active);
 
 // O resto do ficheiro permanece igual, mas é incluído aqui para ser completo
 
 static int map_value(int value, int in_min, int in_max, int out_min, int out_max) {
     if (in_max == in_min) return out_min;
     return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
 }
 
 static void draw_channel_graph_background() {
     int y_base = GRAPH_Y + GRAPH_HEIGHT;
     for (int rssi = -40; rssi >= -90; rssi -= 10) {
         int y_pos = map_value(rssi, -95, -30, y_base, GRAPH_Y);
         st7789_draw_hline_fb(GRAPH_X, y_pos, GRAPH_WIDTH, COLOR_GRID);
         char rssi_str[5];
         snprintf(rssi_str, sizeof(rssi_str), "%d", rssi);
         st7789_draw_text_fb(0, y_pos - 4, rssi_str, COLOR_TEXT_SECONDARY, COLOR_BACKGROUND);
     }
     for (int ch = 1; ch <= 14; ++ch) {
         int x_pos = map_value(ch, 1, 14, GRAPH_X + 5, GRAPH_X + GRAPH_WIDTH - 5);
         st7789_draw_vline_fb(x_pos, GRAPH_Y, GRAPH_HEIGHT, COLOR_GRID);
         if (ch % 2 != 0 || ch == 14) {
              char ch_str[3];
             snprintf(ch_str, sizeof(ch_str), "%d", ch);
             st7789_draw_text_fb(x_pos - ((ch > 9) ? 6 : 3), y_base + 5, ch_str, COLOR_TEXT_SECONDARY, COLOR_BACKGROUND);
         }
     }
     st7789_draw_rect_fb(GRAPH_X, GRAPH_Y, GRAPH_WIDTH, GRAPH_HEIGHT, COLOR_TEXT_SECONDARY);
 }
 
 static void draw_network_curve(const wifi_ap_record_t *ap, uint16_t color, bool is_selected) {
     if (!ap) return;
     int base_width_pixels = 38;
     int x_center = map_value(ap->primary, 1, 14, GRAPH_X + 5, GRAPH_X + GRAPH_WIDTH - 5);
     int y_peak = map_value(ap->rssi, -95, -30, GRAPH_Y + GRAPH_HEIGHT, GRAPH_Y);
     if (y_peak < GRAPH_Y) y_peak = GRAPH_Y;
     int y_base = GRAPH_Y + GRAPH_HEIGHT;
     
     for (int dx = -base_width_pixels / 2; dx <= base_width_pixels / 2; dx++) {
         float normalized_x_sq = powf((float)dx / (base_width_pixels / 2.0f), 2);
         int y_curve = y_peak + (int)((float)(y_base - y_peak) * normalized_x_sq);
         int current_x = x_center + dx;
         if (current_x >= GRAPH_X && current_x < (GRAPH_X + GRAPH_WIDTH) && y_curve < y_base) {
              st7789_draw_vline_fb(current_x, y_curve, y_base - y_curve, color);
         }
     }
 
     if (is_selected) {
         int prev_x = -1, prev_y = -1;
         for (int dx = -base_width_pixels / 2; dx <= base_width_pixels / 2; dx++) {
             float normalized_x_sq = powf((float)dx / (base_width_pixels / 2.0f), 2);
             int y_curve = y_peak + (int)((float)(y_base - y_peak) * normalized_x_sq);
             int current_x = x_center + dx;
             if (prev_x != -1) st7789_draw_line_fb(prev_x, prev_y, current_x, y_curve, COLOR_SELECTED_OUTLINE);
             prev_x = current_x; prev_y = y_curve;
         }
         st7789_draw_hline_fb(x_center - base_width_pixels/2, y_base, base_width_pixels, COLOR_SELECTED_OUTLINE);
     }
 }
 
 static const char* get_auth_mode_string(wifi_auth_mode_t authmode) {
     switch (authmode) {
         case WIFI_AUTH_OPEN: return "Aberta"; case WIFI_AUTH_WEP: return "WEP";
         case WIFI_AUTH_WPA_PSK: return "WPA-PSK"; case WIFI_AUTH_WPA2_PSK: return "WPA2-PSK";
         case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2-PSK"; case WIFI_AUTH_WPA3_PSK: return "WPA3-PSK";
         default: return "Desconhecida";
     }
 }
 
 static void draw_detailed_info_box(const wifi_ap_record_t *ap) {
     uint16_t box_color = 0x18C3;
     st7789_fill_round_rect_fb(5, DETAILS_BOX_Y, 230, DETAILS_BOX_HEIGHT, 5, box_color);
     st7789_draw_round_rect_fb(5, DETAILS_BOX_Y, 230, DETAILS_BOX_HEIGHT, 5, COLOR_HIGHLIGHT);
     char info_line[64];
     st7789_set_text_size(2);
     snprintf(info_line, sizeof(info_line), "%.16s", ap->ssid);
     st7789_draw_text_fb(15, DETAILS_BOX_Y + 8, info_line, COLOR_TEXT_PRIMARY, box_color);
     st7789_set_text_size(1);
     snprintf(info_line, sizeof(info_line), "RSSI: %d dBm | Canal: %d", ap->rssi, ap->primary);
     st7789_draw_text_fb(15, DETAILS_BOX_Y + 30, info_line, COLOR_TEXT_PRIMARY, box_color);
     snprintf(info_line, sizeof(info_line), "Seguranca: %s", get_auth_mode_string(ap->authmode));
     st7789_draw_text_fb(15, DETAILS_BOX_Y + 45, info_line, COLOR_TEXT_PRIMARY, box_color);
 }
 
 static int compare_ap_records(const void *a, const void *b) {
     const wifi_ap_record_t *ap_a = (const wifi_ap_record_t *)a;
     const wifi_ap_record_t *ap_b = (const wifi_ap_record_t *)b;
     if (ap_a->rssi < ap_b->rssi) return -1;
     if (ap_a->rssi > ap_b->rssi) return 1;
     return 0;
 }
 
 static const char* get_cipher_string(wifi_cipher_type_t cipher) {
     switch (cipher) {
         case WIFI_CIPHER_TYPE_NONE: return "Nenhuma"; case WIFI_CIPHER_TYPE_WEP40: return "WEP40";
         case WIFI_CIPHER_TYPE_WEP104: return "WEP104"; case WIFI_CIPHER_TYPE_TKIP: return "TKIP";
         case WIFI_CIPHER_TYPE_CCMP: return "CCMP (AES)"; case WIFI_CIPHER_TYPE_TKIP_CCMP: return "TKIP+CCMP";
         default: return "Desconhecido";
     }
 }
 
 static void draw_signal_bars(int x, int y, int rssi) {
     int num_bars = 4;
     int bar_width = 6;
     int bar_spacing = 3;
     int active_bars;
 
     if (rssi > -56) active_bars = 4;
     else if (rssi > -66) active_bars = 3;
     else if (rssi > -76) active_bars = 2;
     else if (rssi > -86) active_bars = 1;
     else active_bars = 0;
 
     for (int i = 0; i < num_bars; i++) {
         int bar_height = 6 + (i * 5);
         uint16_t color = (i < active_bars) ? ST7789_COLOR_LIME : COLOR_GRID;
         st7789_fill_rect_fb(x + i * (bar_width + bar_spacing), y - bar_height, bar_width, bar_height, color);
     }
 }
 
 static void draw_lock_icon(int x, int y, uint16_t color) {
     st7789_draw_round_rect_fb(x, y, 14, 10, 4, color);
     st7789_fill_rect_fb(x + 1, y + 1, 12, 4, COLOR_BACKGROUND);
     st7789_fill_rect_fb(x + 4, y + 10, 6, 2, COLOR_BACKGROUND);
     st7789_fill_round_rect_fb(x - 2, y + 11, 18, 12, 3, color);
 }
 
 static int draw_standard_tag(int x, int y, const char* label, bool is_active) {
     uint16_t text_color = is_active ? COLOR_BACKGROUND : COLOR_TEXT_SECONDARY;
     uint16_t bg_color = is_active ? ST7789_COLOR_CYAN : COLOR_GRID;
     int text_width = strlen(label) * 6;
     int tag_width = text_width + 10;
     
     st7789_fill_round_rect_fb(x, y, tag_width, 14, 5, bg_color);
     st7789_draw_text_fb(x + 5, y + 4, label, text_color, bg_color);
     
     return x + tag_width + 6;
 }
 
 static void show_detailed_network_view(const wifi_ap_record_t *ap) {
     if (!ap) return;
 
     st7789_fill_screen_fb(COLOR_BACKGROUND);
     char buffer[64];
     int y_pos = 45;
 
     st7789_set_text_size(2);
     snprintf(buffer, sizeof(buffer), "%.18s", ap->ssid);
     st7789_draw_text_centered(120, 15, buffer, COLOR_TEXT_PRIMARY);
     st7789_set_text_size(1);
     snprintf(buffer, sizeof(buffer), "%02x:%02x:%02x:%02x:%02x:%02x",
              ap->bssid[0], ap->bssid[1], ap->bssid[2], ap->bssid[3], ap->bssid[4], ap->bssid[5]);
     st7789_draw_text_centered(120, y_pos, buffer, COLOR_TEXT_SECONDARY);
     y_pos += 20;
     st7789_draw_hline_fb(10, y_pos, 220, COLOR_DIVIDER);
     y_pos += 15;
 
     draw_signal_bars(20, y_pos + 15, ap->rssi);
     snprintf(buffer, sizeof(buffer), "%d dBm", ap->rssi);
     st7789_draw_text_fb(60, y_pos, buffer, COLOR_TEXT_PRIMARY, COLOR_BACKGROUND);
 
     st7789_draw_text_fb(150, y_pos, "Canal:", COLOR_TEXT_SECONDARY, COLOR_BACKGROUND);
     snprintf(buffer, sizeof(buffer), "%d", ap->primary);
     st7789_set_text_size(2);
     st7789_draw_text_fb(200, y_pos - 4, buffer, COLOR_TEXT_PRIMARY, COLOR_BACKGROUND);
     st7789_set_text_size(1);
     y_pos += 30;
     st7789_draw_hline_fb(10, y_pos, 220, COLOR_DIVIDER);
     y_pos += 15;
 
     draw_lock_icon(20, y_pos, COLOR_TEXT_SECONDARY);
     st7789_draw_text_fb(50, y_pos, get_auth_mode_string(ap->authmode), COLOR_TEXT_PRIMARY, COLOR_BACKGROUND);
     y_pos += 15;
     snprintf(buffer, sizeof(buffer), "Cifras: %s + %s", get_cipher_string(ap->pairwise_cipher), get_cipher_string(ap->group_cipher));
     st7789_draw_text_fb(50, y_pos, buffer, COLOR_TEXT_SECONDARY, COLOR_BACKGROUND);
     y_pos += 25;
     st7789_draw_hline_fb(10, y_pos, 220, COLOR_DIVIDER);
     y_pos += 15;
 
     int x_pos = 20;
     x_pos = draw_standard_tag(x_pos, y_pos, "802.11b", ap->phy_11b);
     x_pos = draw_standard_tag(x_pos, y_pos, "802.11g", ap->phy_11g);
     draw_standard_tag(x_pos, y_pos, "802.11n", ap->phy_11n);
     
     st7789_draw_text_centered(120, 225, "Pressione BACK para voltar", COLOR_TEXT_SECONDARY);
     st7789_flush();
 
     while (true) {
         if (!gpio_get_level(BTN_BACK)) {
             while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(20));
             break;
         }
         vTaskDelay(pdMS_TO_TICKS(50));
     }
 }
 
 void show_wifi_analyzer(void) {
     st7789_fill_screen_fb(COLOR_BACKGROUND);
     st7789_set_text_size(2);
     st7789_draw_text_centered(120, 110, "A procurar...", COLOR_TEXT_PRIMARY);
     st7789_flush();
 
     wifi_service_scan();
     uint16_t ap_count = wifi_service_get_ap_count();
     
     wifi_ap_record_t local_aps[WIFI_SCAN_LIST_SIZE];
     if (ap_count > 0) {
         memcpy(local_aps, wifi_service_get_ap_record(0), ap_count * sizeof(wifi_ap_record_t));
         qsort(local_aps, ap_count, sizeof(wifi_ap_record_t), compare_ap_records);
     }
 
     int selected_ap = ap_count > 0 ? ap_count - 1 : 0;
     bool running = true;
     bool needs_redraw = true;
 
     while (running) {
         if (needs_redraw) {
             st7789_fill_screen_fb(COLOR_BACKGROUND);
             st7789_set_text_size(1);
             st7789_draw_text_fb(10, 10, "Analisador WiFi 2.4Ghz", COLOR_HIGHLIGHT, COLOR_BACKGROUND);
             st7789_draw_hline_fb(10, 25, 220, COLOR_HIGHLIGHT);
             draw_channel_graph_background();
 
             if (ap_count > 0) {
                 for (int i = 0; i < ap_count; ++i) {
                     if (i == selected_ap) continue;
                     uint16_t color = GRAPH_COLORS[i % NUM_GRAPH_COLORS];
                     draw_network_curve(&local_aps[i], color, false);
                 }
                 uint16_t selected_color = GRAPH_COLORS[selected_ap % NUM_GRAPH_COLORS];
                 draw_network_curve(&local_aps[selected_ap], selected_color, true);
                 draw_detailed_info_box(&local_aps[selected_ap]);
             } else {
                 st7789_set_text_size(2);
                 st7789_draw_text_centered(120, 110, "Nenhuma rede!", COLOR_TEXT_SECONDARY);
             }
             st7789_flush();
             needs_redraw = false;
         }
 
         if (!gpio_get_level(BTN_DOWN)) {
             while(!gpio_get_level(BTN_DOWN)) vTaskDelay(pdMS_TO_TICKS(10));
             if (ap_count > 0) { selected_ap = (selected_ap > 0) ? selected_ap - 1 : ap_count - 1; needs_redraw = true; }
         } else if (!gpio_get_level(BTN_UP)) {
             while(!gpio_get_level(BTN_UP)) vTaskDelay(pdMS_TO_TICKS(10));
             if (ap_count > 0) { selected_ap = (selected_ap < ap_count - 1) ? selected_ap + 1 : 0; needs_redraw = true; }
         } else if (!gpio_get_level(BTN_OK)) {
             while(!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(20));
             if (ap_count > 0) {
                 show_detailed_network_view(&local_aps[selected_ap]);
                 needs_redraw = true;
             }
         } else if (!gpio_get_level(BTN_BACK)) {
             while(!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(10));
             running = false;
         }
         
         vTaskDelay(pdMS_TO_TICKS(50));
     }
 }
