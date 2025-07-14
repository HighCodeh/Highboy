#include "buzzer_ui.h"
#include "st7789.h"
#include "buzzer.h"
#include "pin_def.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

// --- Estado da UI ---
static uint8_t volume = 128;       // Volume inicial (0-255)
static uint16_t frequency = 1000;  // Frequência inicial em Hz

typedef enum {
    CONTROL_VOLUME,
    CONTROL_FREQUENCY,
} active_control_t;

static active_control_t current_control = CONTROL_VOLUME;

// --- Funções de Desenho ---

/**
 * @brief Desenha uma barra de controle na tela.
 */
static void draw_control_bar(int y, const char* label, uint32_t value, bool is_selected) {
    const int BAR_X = 50;
    const int BAR_WIDTH = 180;
    const int BAR_HEIGHT = 25;
    // Define o valor máximo para a barra baseado no rótulo
    uint32_t max_value = (strcmp(label, "Vol") == 0) ? 255 : 8000;

    // Desenha o rótulo e o valor numérico
    st7789_draw_text_fb(5, y + 8, label, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    char value_str[6];
    snprintf(value_str, 6, "%lu", value);
    st7789_draw_text_fb(BAR_X + BAR_WIDTH + 8, y + 8, value_str, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);

    // Desenha a barra
    st7789_draw_rect_fb(BAR_X, y, BAR_WIDTH, BAR_HEIGHT, ST7789_COLOR_GRAY);
    int fill_width = (value * (BAR_WIDTH - 4)) / max_value;
    if (fill_width > 0) {
        st7789_fill_rect_fb(BAR_X + 2, y + 2, fill_width, BAR_HEIGHT - 4, ST7789_COLOR_PURPLE);
    }

    // Desenha o indicador de seleção
    if (is_selected) {
        st7789_draw_text_fb(0, y + 8, ">", ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK);
    } else {
        st7789_draw_text_fb(0, y + 8, " ", ST7789_COLOR_BLACK, ST7789_COLOR_BLACK);
    }
}

/**
 * @brief Desenha a interface completa na tela.
 */
static void draw_buzzer_ui(void) {
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    st7789_draw_text_fb(50, 20, "Controle do Buzzer", ST7789_COLOR_PURPLE, ST7789_COLOR_BLACK);
    st7789_draw_text_fb(10, 50, "OK: Alterna | CIMA/BAIXO: Ajusta", ST7789_COLOR_GRAY, ST7789_COLOR_BLACK);
    
    draw_control_bar(80, "Vol", volume, current_control == CONTROL_VOLUME);
    draw_control_bar(140, "Freq", frequency, current_control == CONTROL_FREQUENCY);
    
    st7789_flush();
}

// --- Função Principal da UI ---

static bool button_pressed(gpio_num_t pin) {
    return gpio_get_level(pin) == 0;
}

void show_buzzer_screen(void) {
    draw_buzzer_ui();
    buzzer_start_tone(frequency, volume); // Liga o som ao entrar na tela

    // Espera o botão OK (que abriu o menu) ser solto
    while (button_pressed(BTN_OK)) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    while (1) {
        bool changed = false;

        // Botão OK: alterna entre os controles
        if (button_pressed(BTN_OK)) {
            current_control = (current_control == CONTROL_VOLUME) ? CONTROL_FREQUENCY : CONTROL_VOLUME;
            changed = true;
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        // Botões CIMA/BAIXO: ajustam o valor selecionado
        if (button_pressed(BTN_UP)) {
            if (current_control == CONTROL_VOLUME) {
                if (volume <= 245) volume += 10; else volume = 255;
            } else { // Controle de Frequência
                if (frequency <= 7900) frequency += 100; else frequency = 8000;
            }
            changed = true;
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        if (button_pressed(BTN_DOWN)) {
            if (current_control == CONTROL_VOLUME) {
                if (volume >= 10) volume -= 10; else volume = 0;
            } else { // Controle de Frequência
                if (frequency >= 200) frequency -= 100; else frequency = 100;
            }
            changed = true;
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        // Se algo mudou, atualiza o som e a tela
        if (changed) {
            buzzer_start_tone(frequency, volume); // Atualiza o som em tempo real
            draw_buzzer_ui();
        }

        // Botão VOLTAR: sai da tela
        if (button_pressed(BTN_BACK)) {
            buzzer_stop_tone(); // ESSENCIAL: para o som ao sair da tela
            vTaskDelay(pdMS_TO_TICKS(200));
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}