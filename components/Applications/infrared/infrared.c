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

#include "infrared.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "st7789.h"
#include "sub_menu.h"
#include "pin_def.h"
#include "ir_common.h"
#include "ir_storage.h"
#include "ir_burst.h"
#include <math.h>
#include <string.h>
#include <dirent.h>

static const char *TAG = "infrared";

// ============================================================================
// TEMA ROXINHO RETRO
// ============================================================================

#define PURPLE_DARK       0x4010
#define PURPLE_MAIN       0x780F
#define PURPLE_LIGHT      0xA81F
#define PURPLE_ACCENT     0xF81F
#define BG_BLACK          0x0000
#define TEXT_WHITE        0xFFFF
#define TEXT_GRAY         0x7BEF

#define COLOR_LEARN       0x07E0
#define COLOR_TX          0xF800
#define COLOR_BURST       0xFFE0

// ============================================================================
// ESTRUTURAS
// ============================================================================

#define MAX_FILES         32
#define MAX_FILENAME      64
#define VISIBLE_LINES     7

typedef struct {
    char filename[MAX_FILENAME];
} ir_file_entry_t;

typedef struct {
    ir_file_entry_t files[MAX_FILES];
    int count;
    int selected;
    int scroll_offset;
} ir_browser_t;

static ir_browser_t g_browser = {0};
static bool g_task_running = false;
static TaskHandle_t g_animation_task = NULL;

// ============================================================================
// PROTÓTIPOS
// ============================================================================

static void ir_action_learn(void);
static void ir_action_browser(void);
static void ir_action_burst(void);

static bool load_ir_files(void);
static void draw_browser(void);
static void browser_transmit_file(const char *filename);
static void draw_retro_learn_ui(uint32_t elapsed_ms, uint32_t total_ms);
static void draw_retro_burst_ui(int current, int total);

// ============================================================================
// MENU
// ============================================================================

static const SubMenuItem infraredMenuItems[] = {
    { "Learn Signal", NULL, ir_action_learn },
    { "Browse & Send", NULL, ir_action_browser },
    { "Burst All", NULL, ir_action_burst },
};

static const int infraredMenuSize = sizeof(infraredMenuItems) / sizeof(SubMenuItem);

void show_infrared_menu(void) {
    ESP_LOGI(TAG, "Menu Infravermelho");
    show_submenu(infraredMenuItems, infraredMenuSize, "Infrared");
}

// ============================================================================
// BROWSER DE ARQUIVOS - ESTILO CRT RETRO
// ============================================================================

static bool load_ir_files(void) {
    g_browser.count = 0;
    g_browser.selected = 0;
    g_browser.scroll_offset = 0;
    
    DIR *dir = opendir(IR_STORAGE_BASE_PATH);
    if (!dir) {
        ESP_LOGE(TAG, "Falha ao abrir diretório IR");
        return false;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && g_browser.count < MAX_FILES) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        size_t len = strlen(entry->d_name);
        if (len > 3) {
            const char *ext = entry->d_name + len - 3;
            if (strcasecmp(ext, ".ir") == 0) {
                strncpy(g_browser.files[g_browser.count].filename, 
                       entry->d_name, len - 3);
                g_browser.files[g_browser.count].filename[len - 3] = '\0';
                g_browser.count++;
            }
        }
    }
    closedir(dir);
    
    ESP_LOGI(TAG, "Encontrados %d arquivos IR", g_browser.count);
    return g_browser.count > 0;
}

static void draw_browser(void) {
    st7789_fill_screen_fb(BG_BLACK);
    
    // ═══════════════════════════════════════
    // HEADER ESTILO TERMINAL RETRO
    // ═══════════════════════════════════════
    st7789_fill_rect_fb(0, 0, 240, 50, PURPLE_DARK);
    
    // Borda superior dupla
    for (int i = 0; i < 240; i += 4) {
        st7789_draw_pixel_fb(i, 48, PURPLE_ACCENT);
        st7789_draw_pixel_fb(i+1, 48, PURPLE_ACCENT);
    }
    
    st7789_set_text_size(3);
    st7789_draw_text_fb(25, 15, "IR FILES", TEXT_WHITE, PURPLE_DARK);
    st7789_set_text_size(1);
    
    // Badge contador
    char badge[16];
    snprintf(badge, sizeof(badge), "[%d]", g_browser.count);
    st7789_fill_rect_fb(175, 10, 55, 28, PURPLE_MAIN);
    st7789_set_text_size(2);
    st7789_draw_text_fb(185, 16, badge, PURPLE_ACCENT, PURPLE_MAIN);
    st7789_set_text_size(1);
    
    // ═══════════════════════════════════════
    // LISTA COM SCROLL LIMITADO
    // ═══════════════════════════════════════
    int y = 70;
    int visible_start = g_browser.scroll_offset;
    int visible_end = visible_start + VISIBLE_LINES;
    
    if (visible_end > g_browser.count) {
        visible_end = g_browser.count;
    }
    
    for (int i = visible_start; i < visible_end; i++) {
        if (i == g_browser.selected) {
            // Item selecionado - estilo CRT scanline
            st7789_fill_rect_fb(8, y - 4, 224, 28, PURPLE_MAIN);
            
            // Scanlines horizontais
            for (int sy = y - 4; sy < y + 24; sy += 2) {
                for (int sx = 8; sx < 232; sx += 3) {
                    st7789_draw_pixel_fb(sx, sy, PURPLE_DARK);
                }
            }
            
            // Cursor piscante
            st7789_set_text_size(2);
            st7789_draw_text_fb(15, y + 2, ">", PURPLE_ACCENT, PURPLE_MAIN);
            st7789_draw_text_fb(35, y + 2, g_browser.files[i].filename, TEXT_WHITE, PURPLE_MAIN);
            st7789_set_text_size(1);
        } else {
            // Item normal
            st7789_set_text_size(2);
            st7789_draw_text_fb(20, y + 2, g_browser.files[i].filename, TEXT_GRAY, BG_BLACK);
            st7789_set_text_size(1);
        }
        
        y += 30;
    }
    
    // ═══════════════════════════════════════
    // SCROLLBAR RETRÔ (SÓ APARECE SE NECESSÁRIO)
    // ═══════════════════════════════════════
    if (g_browser.count > VISIBLE_LINES) {
        int scrollbar_h = (VISIBLE_LINES * 210) / g_browser.count;
        int scrollbar_y = 70 + (g_browser.scroll_offset * 210) / g_browser.count;
        
        // Background da scrollbar
        st7789_fill_rect_fb(234, 70, 4, 210, PURPLE_DARK);
        
        // Thumb da scrollbar
        st7789_fill_rect_fb(234, scrollbar_y, 4, scrollbar_h, PURPLE_ACCENT);
    }
    
    // ═══════════════════════════════════════
    // FOOTER COM INSTRUÇÕES
    // ═══════════════════════════════════════
    st7789_fill_rect_fb(0, 285, 240, 35, PURPLE_DARK);
    
    // Borda inferior dupla
    for (int i = 0; i < 240; i += 4) {
        st7789_draw_pixel_fb(i, 285, PURPLE_ACCENT);
        st7789_draw_pixel_fb(i+1, 285, PURPLE_ACCENT);
    }
    
    st7789_set_text_size(1);
    st7789_draw_text_fb(15, 295, "OK: SEND x10", PURPLE_LIGHT, PURPLE_DARK);
    st7789_draw_text_fb(15, 305, "BACK: EXIT", TEXT_GRAY, PURPLE_DARK);
    
    st7789_flush();
}

// ============================================================================
// TELA DE RECEIVE - MINIMALISTA RETRO
// ============================================================================

static void draw_retro_learn_ui(uint32_t elapsed_ms, uint32_t total_ms) {
    st7789_fill_screen_fb(BG_BLACK);
    
    // Header simples
    st7789_fill_rect_fb(0, 0, 240, 45, PURPLE_MAIN);
    st7789_set_text_size(3);
    st7789_draw_text_fb(45, 12, "LEARN", TEXT_WHITE, PURPLE_MAIN);
    st7789_set_text_size(1);
    
    // LED indicador pulsante
    uint16_t led_color = ((elapsed_ms / 500) % 2) ? COLOR_LEARN : PURPLE_DARK;
    st7789_fill_circle_fb(120, 80, 25, led_color);
    st7789_fill_circle_fb(120, 80, 15, BG_BLACK);
    st7789_fill_circle_fb(120, 80, 10, led_color);
    
    // Status
    st7789_set_text_size(2);
    const char *msg = ((elapsed_ms / 500) % 2) ? "LISTENING..." : "READY";
    int msg_len = strlen(msg) * 12;
    st7789_draw_text_fb((240 - msg_len) / 2, 130, msg, TEXT_WHITE, BG_BLACK);
    st7789_set_text_size(1);
    
    // Timer circular
    int progress = (elapsed_ms * 100) / total_ms;
    int remaining_sec = (total_ms - elapsed_ms) / 1000;
    
    char timer_txt[16];
    snprintf(timer_txt, sizeof(timer_txt), "%ds", remaining_sec);
    st7789_set_text_size(4);
    int timer_len = strlen(timer_txt) * 24;
    st7789_draw_text_fb((240 - timer_len) / 2, 170, timer_txt, PURPLE_ACCENT, BG_BLACK);
    st7789_set_text_size(1);
    
    // Progress bar minimalista
    int bar_w = (200 * progress) / 100;
    st7789_fill_rect_fb(20, 240, 200, 6, PURPLE_DARK);
    st7789_fill_rect_fb(20, 240, bar_w, 6, COLOR_LEARN);
    
    // Instruções
    st7789_draw_text_fb(40, 270, "Point remote & press", TEXT_GRAY, BG_BLACK);
    st7789_draw_text_fb(60, 285, "BACK to cancel", TEXT_GRAY, BG_BLACK);
    
    st7789_flush();
}

// ============================================================================
// TELA DE BURST - SIMPLES E FUNCIONAL
// ============================================================================

static void draw_retro_burst_ui(int current, int total) {
    st7789_fill_screen_fb(BG_BLACK);
    
    // Header
    st7789_fill_rect_fb(0, 0, 240, 45, COLOR_BURST);
    st7789_set_text_size(3);
    st7789_draw_text_fb(50, 12, "BURST", BG_BLACK, COLOR_BURST);
    st7789_set_text_size(1);
    
    // Contador GIGANTE
    char counter[24];
    snprintf(counter, sizeof(counter), "%d/%d", current, total);
    st7789_set_text_size(5);
    int cnt_len = strlen(counter) * 30;
    st7789_draw_text_fb((240 - cnt_len) / 2, 100, counter, COLOR_BURST, BG_BLACK);
    st7789_set_text_size(1);
    
    // Label
    st7789_set_text_size(2);
    st7789_draw_text_fb(60, 180, "FILES SENT", TEXT_GRAY, BG_BLACK);
    st7789_set_text_size(1);
    
    // Progress bar
    int progress = (current * 100) / total;
    int bar_w = (200 * progress) / 100;
    st7789_fill_rect_fb(20, 220, 200, 10, PURPLE_DARK);
    st7789_fill_rect_fb(20, 220, bar_w, 10, COLOR_BURST);
    
    char perc[8];
    snprintf(perc, sizeof(perc), "%d%%", progress);
    st7789_set_text_size(2);
    st7789_draw_text_fb(95, 218, perc, TEXT_WHITE, BG_BLACK);
    st7789_set_text_size(1);
    
    // Indicador de transmissão
    if ((current % 2) == 0) {
        st7789_fill_circle_fb(20, 260, 8, COLOR_TX);
    } else {
        st7789_fill_circle_fb(20, 260, 8, PURPLE_DARK);
    }
    st7789_draw_text_fb(35, 255, "TRANSMITTING...", TEXT_GRAY, BG_BLACK);
    
    // Instruções
    st7789_draw_text_fb(60, 285, "BACK to cancel", TEXT_GRAY, BG_BLACK);
    
    st7789_flush();
}

// ============================================================================
// TRANSMISSÃO 10x NO BROWSER
// ============================================================================

static void browser_transmit_file(const char *filename) {
    ESP_LOGI(TAG, "Transmitindo 10x: %s", filename);
    
    for (int i = 0; i < 10; i++) {
        if (!gpio_get_level(BTN_BACK)) {
            ESP_LOGW(TAG, "Cancelado");
            break;
        }
        
        // UI simples
        st7789_fill_screen_fb(BG_BLACK);
        
        st7789_fill_rect_fb(0, 0, 240, 45, COLOR_TX);
        st7789_set_text_size(3);
        st7789_draw_text_fb(25, 12, "SENDING", TEXT_WHITE, COLOR_TX);
        st7789_set_text_size(1);
        
        char counter[16];
        snprintf(counter, sizeof(counter), "%d/10", i + 1);
        st7789_set_text_size(5);
        int len = strlen(counter) * 30;
        st7789_draw_text_fb((240 - len) / 2, 100, counter, COLOR_TX, BG_BLACK);
        st7789_set_text_size(1);
        
        // Progress
        int progress = ((i + 1) * 100) / 10;
        int bar_w = (200 * progress) / 100;
        st7789_fill_rect_fb(20, 200, 200, 10, PURPLE_DARK);
        st7789_fill_rect_fb(20, 200, bar_w, 10, COLOR_TX);
        
        st7789_set_text_size(2);
        st7789_draw_text_fb(50, 230, filename, TEXT_GRAY, BG_BLACK);
        st7789_set_text_size(1);
        
        st7789_flush();
        
        ir_tx_send_from_file(filename);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    
    // Tela de conclusão
    st7789_fill_screen_fb(BG_BLACK);
    st7789_set_text_size(3);
    st7789_draw_text_fb(30, 140, "COMPLETE!", COLOR_LEARN, BG_BLACK);
    st7789_flush();
    vTaskDelay(pdMS_TO_TICKS(1000));
}

// ============================================================================
// AÇÕES
// ============================================================================

static void ir_action_learn(void) {
    ESP_LOGI(TAG, "Learn Signal");
    while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(50));
    
    // Sempre salva como "latest"
    const char *filename = "latest";
    uint32_t timeout = 10000;
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    ESP_LOGI(TAG, "Iniciando captura IR...");
    
    // Mostra UI de espera
    draw_retro_learn_ui(0, timeout);
    
    // Chama a função de recepção (BLOQUEANTE)
    ESP_LOGI(TAG, "Chamando ir_receive('%s', %lu)...", filename, timeout);
    bool result = ir_receive(filename, timeout);
    
    uint32_t elapsed = (xTaskGetTickCount() * portTICK_PERIOD_MS) - start_time;
    ESP_LOGI(TAG, "ir_receive retornou: %s (tempo: %lu ms)", result ? "TRUE" : "FALSE", elapsed);
    
    // Tela de resultado
    st7789_fill_screen_fb(BG_BLACK);
    
    if (result) {
        // SUCESSO!
        st7789_fill_rect_fb(0, 0, 240, 45, COLOR_LEARN);
        st7789_set_text_size(3);
        st7789_draw_text_fb(30, 12, "SUCCESS!", TEXT_WHITE, COLOR_LEARN);
        st7789_set_text_size(1);
        
        // Checkmark grande
        st7789_fill_circle_fb(120, 120, 40, COLOR_LEARN);
        st7789_set_text_size(5);
        st7789_draw_text_fb(95, 95, "OK", TEXT_WHITE, COLOR_LEARN);
        st7789_set_text_size(1);
        
        st7789_set_text_size(2);
        st7789_draw_text_fb(30, 200, "Saved as:", TEXT_GRAY, BG_BLACK);
        st7789_draw_text_fb(50, 220, "LATEST", PURPLE_ACCENT, BG_BLACK);
        st7789_set_text_size(1);
        
        ESP_LOGI(TAG, "Sinal recebido e salvo como 'latest'");
    } else {
        // FALHA
        st7789_fill_rect_fb(0, 0, 240, 45, COLOR_TX);
        st7789_set_text_size(3);
        st7789_draw_text_fb(35, 12, "TIMEOUT!", TEXT_WHITE, COLOR_TX);
        st7789_set_text_size(1);
        
        st7789_fill_circle_fb(120, 120, 40, COLOR_TX);
        st7789_set_text_size(5);
        st7789_draw_text_fb(95, 95, "X", TEXT_WHITE, COLOR_TX);
        st7789_set_text_size(1);
        
        st7789_set_text_size(2);
        st7789_draw_text_fb(20, 200, "No signal", TEXT_GRAY, BG_BLACK);
        st7789_draw_text_fb(20, 220, "received", TEXT_GRAY, BG_BLACK);
        st7789_set_text_size(1);
        
        ESP_LOGW(TAG, "Nenhum sinal recebido");
    }
    
    st7789_flush();
    vTaskDelay(pdMS_TO_TICKS(2500));
}

static void ir_action_browser(void) {
    ESP_LOGI(TAG, "Browser");
    while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(50));
    
    if (!load_ir_files()) {
        st7789_fill_screen_fb(BG_BLACK);
        st7789_set_text_size(2);
        st7789_draw_text_fb(20, 100, "No IR files!", TEXT_WHITE, BG_BLACK);
        st7789_flush();
        vTaskDelay(pdMS_TO_TICKS(2000));
        return;
    }
    
    bool browsing = true;
    draw_browser();
    
    while (browsing) {
        if (!gpio_get_level(BTN_UP)) {
            while (!gpio_get_level(BTN_UP)) vTaskDelay(pdMS_TO_TICKS(50));
            if (g_browser.selected > 0) {
                g_browser.selected--;
                // Ajusta scroll apenas se necessário
                if (g_browser.selected < g_browser.scroll_offset) {
                    g_browser.scroll_offset = g_browser.selected;
                }
                draw_browser();
            }
        }
        
        if (!gpio_get_level(BTN_DOWN)) {
            while (!gpio_get_level(BTN_DOWN)) vTaskDelay(pdMS_TO_TICKS(50));
            if (g_browser.selected < g_browser.count - 1) {
                g_browser.selected++;
                // Ajusta scroll apenas se necessário
                if (g_browser.selected >= g_browser.scroll_offset + VISIBLE_LINES) {
                    g_browser.scroll_offset = g_browser.selected - VISIBLE_LINES + 1;
                }
                draw_browser();
            }
        }
        
        if (!gpio_get_level(BTN_OK)) {
            while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(50));
            browser_transmit_file(g_browser.files[g_browser.selected].filename);
            draw_browser();
        }
        
        if (!gpio_get_level(BTN_BACK)) {
            while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(50));
            browsing = false;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void ir_action_burst(void) {
    ESP_LOGI(TAG, "Burst All Files");
    while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(50));
    
    if (!load_ir_files()) {
        st7789_fill_screen_fb(BG_BLACK);
        st7789_set_text_size(2);
        st7789_draw_text_fb(20, 100, "No IR files!", TEXT_WHITE, BG_BLACK);
        st7789_flush();
        vTaskDelay(pdMS_TO_TICKS(2000));
        return;
    }
    
    int total_files = g_browser.count;
    int current_file = 0;
    
    DIR *dir = opendir(IR_STORAGE_BASE_PATH);
    if (!dir) {
        ESP_LOGE(TAG, "Erro ao abrir diretório");
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!gpio_get_level(BTN_BACK)) {
            while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(50));
            ESP_LOGW(TAG, "Burst cancelado");
            break;
        }
        
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        size_t len = strlen(entry->d_name);
        if (len > 3) {
            const char *ext = entry->d_name + len - 3;
            if (strcasecmp(ext, ".ir") == 0) {
                char filename[MAX_FILENAME];
                strncpy(filename, entry->d_name, len - 3);
                filename[len - 3] = '\0';
                
                current_file++;
                
                draw_retro_burst_ui(current_file, total_files);
                
                ir_tx_send_from_file(filename);
                
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }
    }
    closedir(dir);
    
    ESP_LOGI(TAG, "Burst finalizado: %d/%d", current_file, total_files);
    vTaskDelay(pdMS_TO_TICKS(1000));
}
