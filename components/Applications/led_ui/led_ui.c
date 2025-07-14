// Arquivo: led_ui.c (VERSÃO MELHORADA E FUNCIONAL)

#include "led_ui.h"
#include "st7789.h"
#include "led_control.h"
#include "pin_def.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

// --- Estrutura e Lista de Cores Predefinidas ---
typedef struct {
    const char* name;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_preset_t;

static const color_preset_t presets[] = {
    {"Vermelho", 255, 0,   0},
    {"Verde",    0,   255, 0},
    {"Azul",     0,   0,   255},
    {"Roxo",     255, 0,   255},
    {"Ciano",    0,   255, 255},
    {"Amarelo",  255, 255, 0},
    {"Branco",   255, 255, 255},
    {"Laranja",  255, 165, 0},
};
static const int num_presets = sizeof(presets) / sizeof(presets[0]);
static int current_preset_index = 0;

// --- Estado da UI ---
static uint8_t red;
static uint8_t green;
static uint8_t blue;
static uint8_t brightness = 128; // Começa com brilho em 50%

// --- Configura os botões com pull-up ---
static void buttons_init(void) {
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    uint64_t mask = (1ULL << BTN_UP) | (1ULL << BTN_DOWN) | (1ULL << BTN_LEFT) |
                    (1ULL << BTN_RIGHT) | (1ULL << BTN_OK) | (1ULL << BTN_BACK);
    io_conf.pin_bit_mask = mask;
    gpio_config(&io_conf);
}

// --- Aplica a cor predefinida atual ---
static void apply_preset(int index) {
    if (index >= 0 && index < num_presets) {
        red   = presets[index].r;
        green = presets[index].g;
        blue  = presets[index].b;
    }
}

// --- Desenha a barra de brilho ---
static void draw_brightness_bar(const char* label, uint8_t value) {
    const int BAR_X = 40;
    const int BAR_Y = 100;
    const int BAR_WIDTH = 180;
    const int BAR_HEIGHT = 25;

    st7789_draw_text_fb(10, BAR_Y + 8, label, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    st7789_draw_rect_fb(BAR_X, BAR_Y, BAR_WIDTH, BAR_HEIGHT, ST7789_COLOR_GRAY);

    int fill_width = (value * (BAR_WIDTH - 4)) / 255;
    if (fill_width > 0) {
        st7789_fill_rect_fb(BAR_X + 2, BAR_Y + 2, fill_width, BAR_HEIGHT - 4, ST7789_COLOR_WHITE);
    }
}

// --- Desenha a UI completa ---
static void draw_led_ui(void) {
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);

    // Título com nome da cor
    char preset_title[32];
    snprintf(preset_title, sizeof(preset_title), "< %s >", presets[current_preset_index].name);
    st7789_draw_text_fb(50, 20, preset_title, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    st7789_draw_text_fb(10, 50, "CIMA/BAIXO: Brilho", ST7789_COLOR_GRAY, ST7789_COLOR_BLACK);

    draw_brightness_bar("L", brightness);

    uint8_t final_r = (red * brightness) / 255;
    uint8_t final_g = (green * brightness) / 255;
    uint8_t final_b = (blue * brightness) / 255;
    uint16_t preview_color = st7789_rgb_to_color(final_r, final_g, final_b);

    st7789_draw_text_fb(80, 170, "Cor Final", ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    st7789_fill_rect_fb(80, 190, 80, 40, preview_color);
    st7789_draw_rect_fb(80, 190, 80, 40, ST7789_COLOR_WHITE);

    st7789_flush();
}

// --- Lê botão (com lógica de pull-up: pressionado = 0) ---
static bool button_pressed(gpio_num_t pin) {
    return gpio_get_level(pin) == 0;
}

// --- Loop principal da tela do LED ---
void show_led_screen(void) {
    buttons_init(); // ✅ Inicializa botões corretamente

    current_preset_index = 0;
    apply_preset(current_preset_index);
    draw_led_ui();

    // Espera soltar o botão OK antes de entrar no loop
    while (button_pressed(BTN_OK)) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    while (1) {
        bool changed = false;
        const int BRIGHTNESS_STEP = 15;

        if (button_pressed(BTN_RIGHT)) {
            current_preset_index = (current_preset_index + 1) % num_presets;
            apply_preset(current_preset_index);
            changed = true;
            vTaskDelay(pdMS_TO_TICKS(150));
        }
        if (button_pressed(BTN_LEFT)) {
            current_preset_index = (current_preset_index - 1 + num_presets) % num_presets;
            apply_preset(current_preset_index);
            changed = true;
            vTaskDelay(pdMS_TO_TICKS(150));
        }

        if (button_pressed(BTN_UP)) {
            brightness = (brightness + BRIGHTNESS_STEP > 255) ? 255 : brightness + BRIGHTNESS_STEP;
            changed = true;
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        if (button_pressed(BTN_DOWN)) {
            brightness = (brightness < BRIGHTNESS_STEP) ? 0 : brightness - BRIGHTNESS_STEP;
            changed = true;
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        if (changed) {
            uint8_t final_r = (red * brightness) / 255;
            uint8_t final_g = (green * brightness) / 255;
            uint8_t final_b = (blue * brightness) / 255;
            led_set_color(final_r, final_g, final_b);
            draw_led_ui();
        }

        if (button_pressed(BTN_BACK)) {
            vTaskDelay(pdMS_TO_TICKS(200));
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
