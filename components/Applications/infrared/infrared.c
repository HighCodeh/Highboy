#include "infrared.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "st7789.h"
#include "sub_menu.h"
#include "pin_def.h"
#include <math.h>
#include <string.h>

static const char *TAG = "infrared";

// ============================================================================
// TEMA ROXINHO - IGUAL AOS OUTROS MENUS
// ============================================================================

#define PURPLE_DARK       0x4010  // Roxo escuro
#define PURPLE_MAIN       0x780F  // Roxo principal (magenta)
#define PURPLE_LIGHT      0xA81F  // Roxo claro
#define PURPLE_ACCENT     0xF81F  // Rosa/Magenta brilhante
#define BG_BLACK          0x0000  // Fundo preto
#define TEXT_WHITE        0xFFFF  // Texto branco
#define TEXT_GRAY         0x7BEF  // Texto cinza

// Cores para diferentes modos
#define COLOR_LEARN       0x07E0  // Verde
#define COLOR_TX          0xF800  // Vermelho  
#define COLOR_YELLOW      0xFFE0  // Amarelo

// ============================================================================
// LAYOUT
// ============================================================================

#define WAVE_Y            160    // Onda bem embaixo
#define WAVE_HEIGHT       60     // Altura maior para onda

// ============================================================================
// ESTRUTURAS
// ============================================================================

typedef enum {
    IR_MODE_IDLE,
    IR_MODE_LEARNING,
    IR_MODE_TRANSMITTING,
    IR_MODE_TV_B_GONE,
    IR_MODE_BRUTE_FORCE
} ir_mode_t;

typedef struct {
    ir_mode_t mode;
    uint32_t signal_count;
    bool active;
    char status_msg[40];
} ir_status_t;

static ir_status_t g_ir_status = {0};
static bool g_task_running = false;
static TaskHandle_t g_animation_task = NULL;

// ============================================================================
// PROTÓTIPOS
// ============================================================================

static void ir_action_learn(void);
static void ir_action_transmit(void);
static void ir_action_tv_b_gone(void);
static void ir_action_brute_force(void);

static void draw_gameboy_ui(const char *title, uint16_t mode_color);
static void draw_big_wave(int *wave_data, int size, uint16_t color);
static void generate_wave(int *buffer, int size, int frame, int style);

// ============================================================================
// MENU
// ============================================================================

static const SubMenuItem infraredMenuItems[] = {
    { "Learn Signal", NULL, ir_action_learn },
    { "Transmit", NULL, ir_action_transmit },
    { "TV B Gone", NULL, ir_action_tv_b_gone },
    { "Brute Force", NULL, ir_action_brute_force },
};

static const int infraredMenuSize = sizeof(infraredMenuItems) / sizeof(SubMenuItem);

void show_infrared_menu(void) {
    ESP_LOGI(TAG, "Menu Infravermelho");
    memset(&g_ir_status, 0, sizeof(ir_status_t));
    show_submenu(infraredMenuItems, infraredMenuSize, "Infrared");
}

// ============================================================================
// UI GAME BOY STYLE COM TEMA ROXO
// ============================================================================

static void draw_gameboy_ui(const char *title, uint16_t mode_color) {
    // Fundo preto
    st7789_fill_screen_fb(BG_BLACK);
    
    // ═══════════════════════════════════════
    // BARRA SUPERIOR ROXA (ESTILO GAME BOY)
    // ═══════════════════════════════════════
    st7789_fill_rect_fb(0, 0, 240, 40, PURPLE_MAIN);
    
    // Título GRANDE
    st7789_set_text_size(3);
    int title_len = strlen(title) * 18; // 18px por char em size 3
    int title_x = (240 - title_len) / 2;
    st7789_draw_text_fb(title_x, 10, title, TEXT_WHITE, PURPLE_MAIN);
    st7789_set_text_size(1);
    
    // ═══════════════════════════════════════
    // STATUS - LETRAS GRANDES E CLARAS
    // ═══════════════════════════════════════
    
    // LED de status (canto superior direito)
    if (g_ir_status.active) {
        st7789_fill_circle_fb(15, 55, 8, mode_color);
        st7789_set_text_size(2);
        st7789_draw_text_fb(30, 48, "ATIVO", mode_color, BG_BLACK);
    } else {
        st7789_fill_circle_fb(15, 55, 8, PURPLE_DARK);
        st7789_set_text_size(2);
        st7789_draw_text_fb(30, 48, "PRONTO", PURPLE_DARK, BG_BLACK);
    }
    st7789_set_text_size(1);
    
    // Mensagem de status - BEM GRANDE
    st7789_set_text_size(2);
    int msg_len = strlen(g_ir_status.status_msg) * 12;
    int msg_x = (240 - msg_len) / 2;
    st7789_draw_text_fb(msg_x, 85, g_ir_status.status_msg, TEXT_WHITE, BG_BLACK);
    st7789_set_text_size(1);
    
    // Contador GIGANTE se tiver sinais
    if (g_ir_status.signal_count > 0) {
        char count[16];
        snprintf(count, sizeof(count), "%lu", g_ir_status.signal_count);
        
        st7789_set_text_size(4);
        int count_len = strlen(count) * 24; // 24px por char em size 4
        int count_x = (240 - count_len) / 2;
        st7789_draw_text_fb(count_x, 115, count, PURPLE_ACCENT, BG_BLACK);
        st7789_set_text_size(1);
    }
    
    // ═══════════════════════════════════════
    // ÁREA DA ONDA (EMBAIXO)
    // ═══════════════════════════════════════
    
    // Borda roxa decorativa
    st7789_fill_rect_fb(0, WAVE_Y - 5, 240, 3, PURPLE_MAIN);
    
    // Label "SINAL IR"
    st7789_set_text_size(1);
    st7789_draw_text_fb(10, WAVE_Y - 18, "SINAL IR:", PURPLE_LIGHT, BG_BLACK);
}

static void draw_big_wave(int *wave_data, int size, uint16_t color) {
    // Limpa área da onda
    st7789_fill_rect_fb(0, WAVE_Y, 240, WAVE_HEIGHT, BG_BLACK);
    
    int wave_center = WAVE_Y + (WAVE_HEIGHT / 2);
    
    // Linha de referência pontilhada
    for (int x = 0; x < 240; x += 6) {
        st7789_draw_pixel_fb(x, wave_center, PURPLE_DARK);
        st7789_draw_pixel_fb(x + 1, wave_center, PURPLE_DARK);
    }
    
    // Desenha onda com linhas GROSSAS (estilo Game Boy)
    int x_step = 240 / size;
    
    for (int i = 0; i < size - 1; i++) {
        int x1 = i * x_step;
        int x2 = (i + 1) * x_step;
        
        // Normaliza altura (0-100 -> pixels)
        int h1 = (wave_data[i] * WAVE_HEIGHT) / 200;
        int h2 = (wave_data[i + 1] * WAVE_HEIGHT) / 200;
        
        int y1 = wave_center - h1;
        int y2 = wave_center - h2;
        
        // Linhas GROSSAS (3 pixels de largura)
        st7789_draw_line_fb(x1, y1, x2, y2, color);
        st7789_draw_line_fb(x1, y1 + 1, x2, y2 + 1, color);
        st7789_draw_line_fb(x1, y1 + 2, x2, y2 + 2, color);
    }
}

// ============================================================================
// GERADORES DE ONDA
// ============================================================================

static void generate_wave(int *buffer, int size, int frame, int style) {
    switch(style) {
        case 0: // LEARN - Pulsos de recepção
            for (int i = 0; i < size; i++) {
                int pos = (i + frame * 4) % size;
                if (pos < size / 8) {
                    buffer[i] = 85 + (rand() % 15);
                } else if (pos < size / 6) {
                    buffer[i] = 50 + (rand() % 20);
                } else {
                    buffer[i] = 5 + (rand() % 10);
                }
            }
            break;
            
        case 1: // TRANSMIT - Onda quadrada forte
            for (int i = 0; i < size; i++) {
                int cycle = (i + frame * 3) % 16;
                buffer[i] = (cycle < 8) ? 95 : 5;
            }
            break;
            
        case 2: // TV B-GONE - Rajadas rápidas
            for (int i = 0; i < size; i++) {
                int burst = (i + frame * 5) % 50;
                if (burst < 30) {
                    buffer[i] = ((i + frame * 2) % 4 < 2) ? 90 : 10;
                } else {
                    buffer[i] = 5;
                }
            }
            break;
            
        case 3: // BRUTE FORCE - Onda variável
            for (int i = 0; i < size; i++) {
                float phase = ((i + frame * 2) % 60) * 0.1f;
                buffer[i] = (int)(50 + 45 * sinf(phase));
            }
            break;
    }
}

// ============================================================================
// ANIMAÇÕES
// ============================================================================

void learn_animation_task(void *pvParameters) {
    int wave[80]; // 80 pontos
    int frame = 0;
    
    while (g_task_running) {
        // Alterna status
        if ((frame % 30) > 15) {
            g_ir_status.active = true;
            strcpy(g_ir_status.status_msg, "RECEBENDO!");
            g_ir_status.signal_count++;
        } else {
            g_ir_status.active = false;
            strcpy(g_ir_status.status_msg, "Aponte o controle");
        }
        
        draw_gameboy_ui("LEARN", COLOR_LEARN);
        generate_wave(wave, 80, frame, 0);
        draw_big_wave(wave, 80, COLOR_LEARN);
        
        st7789_flush();
        frame++;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    g_animation_task = NULL;
    vTaskDelete(NULL);
}

void transmit_animation_task(void *pvParameters) {
    int wave[80];
    int frame = 0;
    
    g_ir_status.active = true;
    strcpy(g_ir_status.status_msg, "ENVIANDO");
    
    while (g_task_running) {
        g_ir_status.signal_count = frame / 3;
        
        draw_gameboy_ui("TRANSMIT", COLOR_TX);
        generate_wave(wave, 80, frame, 1);
        draw_big_wave(wave, 80, COLOR_TX);
        
        st7789_flush();
        frame++;
        vTaskDelay(pdMS_TO_TICKS(80));
    }
    
    g_animation_task = NULL;
    vTaskDelete(NULL);
}

void tv_b_gone_animation_task(void *pvParameters) {
    int wave[80];
    int frame = 0;
    
    g_ir_status.active = true;
    strcpy(g_ir_status.status_msg, "ATACANDO TVs");
    
    while (g_task_running) {
        g_ir_status.signal_count = frame / 2;
        
        draw_gameboy_ui("TV B-GONE", COLOR_TX);
        generate_wave(wave, 80, frame, 2);
        draw_big_wave(wave, 80, COLOR_TX);
        
        // Mensagem extra
        st7789_set_text_size(1);
        st7789_draw_text_fb(60, 150, "Desligando TVs...", TEXT_GRAY, BG_BLACK);
        
        st7789_flush();
        frame++;
        vTaskDelay(pdMS_TO_TICKS(70));
    }
    
    g_animation_task = NULL;
    vTaskDelete(NULL);
}

void brute_force_animation_task(void *pvParameters) {
    int wave[80];
    int frame = 0;
    
    g_ir_status.active = true;
    strcpy(g_ir_status.status_msg, "TESTANDO");
    
    while (g_task_running) {
        g_ir_status.signal_count = frame * 5;
        
        draw_gameboy_ui("BRUTE", COLOR_YELLOW);
        generate_wave(wave, 80, frame, 3);
        draw_big_wave(wave, 80, COLOR_YELLOW);
        
        // Barra de progresso simples
        int prog = (frame * 3) % 100;
        st7789_set_text_size(2);
        char prog_text[16];
        snprintf(prog_text, sizeof(prog_text), "%d%%", prog);
        st7789_draw_text_fb(95, 150, prog_text, TEXT_WHITE, BG_BLACK);
        
        st7789_flush();
        frame++;
        vTaskDelay(pdMS_TO_TICKS(60));
    }
    
    g_animation_task = NULL;
    vTaskDelete(NULL);
}

// ============================================================================
// AÇÕES
// ============================================================================

static void ir_action_learn(void) {
    ESP_LOGI(TAG, "Learn");
    while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(50));
    
    g_ir_status.signal_count = 0;
    g_task_running = true;
    xTaskCreate(learn_animation_task, "learn", 4096, NULL, 5, &g_animation_task);
    
    while (g_task_running) {
        if (!gpio_get_level(BTN_BACK)) {
            while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(50));
            g_task_running = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelay(pdMS_TO_TICKS(200));
}

static void ir_action_transmit(void) {
    ESP_LOGI(TAG, "Transmit");
    while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(50));
    
    g_ir_status.signal_count = 0;
    g_task_running = true;
    xTaskCreate(transmit_animation_task, "tx", 4096, NULL, 5, &g_animation_task);
    
    while (g_task_running) {
        if (!gpio_get_level(BTN_BACK)) {
            while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(50));
            g_task_running = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelay(pdMS_TO_TICKS(200));
}

static void ir_action_tv_b_gone(void) {
    ESP_LOGI(TAG, "TV B-Gone");
    while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(50));
    
    g_ir_status.signal_count = 0;
    g_task_running = true;
    xTaskCreate(tv_b_gone_animation_task, "tvbgone", 4096, NULL, 5, &g_animation_task);
    
    while (g_task_running) {
        if (!gpio_get_level(BTN_BACK)) {
            while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(50));
            g_task_running = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelay(pdMS_TO_TICKS(200));
}

static void ir_action_brute_force(void) {
    ESP_LOGI(TAG, "Brute Force");
    while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(50));
    
    g_ir_status.signal_count = 0;
    g_task_running = true;
    xTaskCreate(brute_force_animation_task, "brute", 4096, NULL, 5, &g_animation_task);
    
    while (g_task_running) {
        if (!gpio_get_level(BTN_BACK)) {
            while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(50));
            g_task_running = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelay(pdMS_TO_TICKS(200));
}