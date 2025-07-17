#include "battery_ui.h"
#include "st7789.h"
#include "bq25896.h" // Inclui o driver BQ25896 atualizado
#include "pin_def.h" // Assumindo que BTN_BACK está definido aqui
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <stdio.h>

// Cores para a barra de bateria
#define BATTERY_COLOR_HIGH ST7789_COLOR_GREEN
#define BATTERY_COLOR_MID  ST7789_COLOR_YELLOW
#define BATTERY_COLOR_LOW  ST7789_COLOR_RED


static void draw_battery_ui(int percentage, uint16_t voltage_mv, bq25896_charge_status_t status) {
    char buffer[50];

    st7789_fill_screen_fb(ST7789_COLOR_BLACK);

    st7789_draw_text_fb(45, 10, "Status da Bateria", ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    st7789_draw_hline_fb(10, 30, 220, ST7789_COLOR_DARKGRAY);

    int bat_x = 20;
    int bat_y = 60;
    int bat_w = 200;
    int bat_h = 80;
    st7789_draw_round_rect_fb(bat_x, bat_y, bat_w, bat_h, 10, ST7789_COLOR_WHITE);

    st7789_fill_rect_fb(bat_x + bat_w, bat_y + 20, 10, bat_h - 40, ST7789_COLOR_WHITE);


    if (percentage > 0) {
        int bar_margin = 8;
        int bar_w = ((bat_w - (bar_margin * 2)) * percentage) / 100;
        int bar_h = bat_h - (bar_margin * 2);
        int bar_x = bat_x + bar_margin;
        int bar_y = bat_y + bar_margin;

        uint16_t bar_color;
        // Usa bq25896_is_charging() que acabamos de criar
        if (bq25896_is_charging()) {
            bar_color = ST7789_COLOR_CYAN; // Cor azul enquanto carrega
        } else {
            if (percentage > 50) bar_color = BATTERY_COLOR_HIGH;
            else if (percentage > 20) bar_color = BATTERY_COLOR_MID;
            else bar_color = BATTERY_COLOR_LOW;
        }
        
        st7789_fill_round_rect_fb(bar_x, bar_y, bar_w, bar_h, 5, bar_color);
    }
    

    sprintf(buffer, "Tensao: %.2f V", voltage_mv / 1000.0f);
    st7789_draw_text_fb(20, 160, buffer, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);


    const char* status_text;
    switch(status) {
        case CHARGE_STATUS_NOT_CHARGING:
            status_text = "Status: Nao esta a carregar";
            break;
        case CHARGE_STATUS_PRECHARGE:
            status_text = "Status: Pre-Carga";
            break;
        case CHARGE_STATUS_FAST_CHARGE:
            status_text = "Status: Carregando";
            break;
        case CHARGE_STATUS_CHARGE_DONE:
            status_text = "Status: Carga Completa";
            break;
        case CHARGE_STATUS_UNKNOWN: // Adicionado o novo status
            status_text = "Status: Desconhecido/Erro";
            break;
        default:
            status_text = "Status: Desconhecido"; // Caso algum outro valor seja retornado
            break;
    }
    st7789_draw_text_fb(20, 180, status_text, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    
    st7789_draw_text_fb(40, 220, "Pressione VOLTAR", ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK);

    st7789_flush();
}


void show_battery_screen(void) {

    while (1) {
         // Chamadas para as funções do driver BQ25896
         uint16_t voltage = bq25896_get_battery_voltage();
        int percentage = bq25896_get_battery_percentage(voltage);
        bq25896_charge_status_t status = bq25896_get_charge_status();

        draw_battery_ui(percentage, voltage, status);

        // Certifique-se de que BTN_BACK está configurado como um pino de entrada com pull-up/down apropriado.
        // Adicione um pequeno debounce para o botão
        if (!gpio_get_level(BTN_BACK)) {
            vTaskDelay(pdMS_TO_TICKS(150)); // Pequeno atraso para debounce
            if (!gpio_get_level(BTN_BACK)) { // Verifica novamente após o debounce
                break; // Sai do loop e retorna para o menu
            }
        }

        // Aguardar antes de atualizar a tela novamente
        vTaskDelay(pdMS_TO_TICKS(500)); // Atualiza a cada 500ms
    }
}