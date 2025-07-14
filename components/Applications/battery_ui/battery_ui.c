#include "battery_ui.h"
#include "st7789.h"
#include "bq25896.h"
#include "pin_def.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <stdio.h>

// Cores para a barra de bateria
#define BATTERY_COLOR_HIGH ST7789_COLOR_GREEN
#define BATTERY_COLOR_MID  ST7789_COLOR_YELLOW
#define BATTERY_COLOR_LOW  ST7789_COLOR_RED

// Função para desenhar a UI da bateria
static void draw_battery_ui(int percentage, uint16_t voltage_mv, bq25896_charge_status_t status) {
    char buffer[50];

    // Limpa a tela
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);

    // 1. Título
    st7789_draw_text_fb(45, 10, "Status da Bateria", ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    st7789_draw_hline_fb(10, 30, 220, ST7789_COLOR_DARKGRAY);

    // 2. Desenha o corpo principal do ícone da bateria
    int bat_x = 20;
    int bat_y = 60;
    int bat_w = 200;
    int bat_h = 80;
    st7789_draw_round_rect_fb(bat_x, bat_y, bat_w, bat_h, 10, ST7789_COLOR_WHITE);
    // Desenha o polo positivo da bateria
    st7789_fill_rect_fb(bat_x + bat_w, bat_y + 20, 10, bat_h - 40, ST7789_COLOR_WHITE);

    // 3. Desenha a barra de carga
    if (percentage > 0) {
        int bar_margin = 8;
        int bar_w = ((bat_w - (bar_margin * 2)) * percentage) / 100;
        int bar_h = bat_h - (bar_margin * 2);
        int bar_x = bat_x + bar_margin;
        int bar_y = bat_y + bar_margin;

        uint16_t bar_color;
        if (bq25896_is_charging()) {
            bar_color = ST7789_COLOR_CYAN; // Cor azul enquanto carrega
        } else {
            if (percentage > 50) bar_color = BATTERY_COLOR_HIGH;
            else if (percentage > 20) bar_color = BATTERY_COLOR_MID;
            else bar_color = BATTERY_COLOR_LOW;
        }
        
        st7789_fill_round_rect_fb(bar_x, bar_y, bar_w, bar_h, 5, bar_color);
    }
    
    // Escreve a percentagem dentro da barra
  

    // 4. Informações de texto
    // Tensão
    sprintf(buffer, "Tensao: %.2f V", voltage_mv / 1000.0f);
    st7789_draw_text_fb(20, 160, buffer, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);

    // Status de carregamento
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
        default:
            status_text = "Status: Desconhecido";
            break;
    }
    st7789_draw_text_fb(20, 180, status_text, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    
    // Dica para voltar
    st7789_draw_text_fb(40, 220, "Pressione VOLTAR", ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK);

    // Envia o framebuffer para a tela
    st7789_flush();
}

// Função principal que mostra a tela e gere o loop
void show_battery_screen(void) {
    // Inicializa o chip da bateria. É seguro chamar isto aqui.
    bq25896_init();

    while (1) {
        // 1. Recolher dados
        uint16_t voltage = bq25896_get_battery_voltage();
        int percentage = bq25896_get_battery_percentage(voltage);
        bq25896_charge_status_t status = bq25896_get_charge_status();

        // 2. Desenhar a interface
        draw_battery_ui(percentage, voltage, status);

        // 3. Verificar se o botão de voltar foi pressionado
        if (!gpio_get_level(BTN_BACK)) {
            // Um pequeno debounce para evitar múltiplas leituras
            vTaskDelay(pdMS_TO_TICKS(150)); 
            break; // Sai do loop e retorna para o menu
        }

        // 4. Aguardar antes de atualizar a tela novamente
        vTaskDelay(pdMS_TO_TICKS(500)); // Atualiza a cada 500ms
    }
}