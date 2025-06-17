#include "menu_battery.h"
#include "st7789.h"
#include "bq25896.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

#define BTN_BACK 7

#define COLOR_PURPLE        0x780F
#define COLOR_LIGHT_PURPLE  0xB81F
#define TEXT_SIZE           2

#define COLOR_GREEN  0x07E0
#define COLOR_YELLOW 0xFFE0
#define COLOR_RED    0xF800
#define COLOR_WHITE  0xFFFF

#define UPDATE_INTERVAL_MS 300 // Faz a leitura a cada 300ms

#define SMALL_CHANGE_THRESHOLD 3 // Muda apenas se for > 3mV ou 3mA

static uint8_t voltage_to_percent_precise(uint16_t mV, bool charge_done) {
    if (charge_done) return 100;
    if (mV >= 4200) return 99;
    if (mV <= 3700) return 0;
    return (uint8_t)(((mV - 3700) * 100) / (4200 - 3700));
}

static uint16_t get_color_by_percent(uint8_t percent) {
    if (percent >= 70) return COLOR_GREEN;
    if (percent >= 40) return COLOR_YELLOW;
    return COLOR_RED;
}

static void clear_area(int x, int y, int w, int h) {
    st7789_draw_filled_rect(x, y, w, h, ST7789_COLOR_BLACK);
}

static void draw_battery_icon(int x, int y, uint8_t percent, bool charging, bool blink) {
    int w = 30;
    int h = 14;
    int pin_w = 4;

    st7789_draw_round_rect(x, y, w, h, 2, COLOR_WHITE);
    st7789_draw_filled_rect(x + w, y + 4, pin_w, h - 8, COLOR_WHITE);

    int fill = (percent * (w - 4)) / 100;
    if (fill > 0) {
        uint16_t color = get_color_by_percent(percent);
        st7789_draw_filled_rect(x + 2, y + 2, fill, h - 4, color);
    }

    if (charging && blink) {
        st7789_draw_line(x + 12, y + 2, x + 17, y + 7, COLOR_LIGHT_PURPLE);
        st7789_draw_line(x + 17, y + 7, x + 14, y + 7, COLOR_LIGHT_PURPLE);
        st7789_draw_line(x + 14, y + 7, x + 19, y + 12, COLOR_LIGHT_PURPLE);
    }
}

void battery_info_screen(void) {
    bq25896_battery_info_t info = {0};
    bq25896_battery_info_t last_info = {0};
    bq25896_chg_status_t charge_status = BQ25896_CHG_STATUS_NOT_CHARGING;
    bq25896_chg_status_t last_charge_status = BQ25896_CHG_STATUS_NOT_CHARGING;

    uint8_t last_percent = 0xFF;
    bool last_charging = false;
    bool blink = true;
    uint32_t last_blink_time = 0;
    char buffer[32];

    st7789_fill_screen(ST7789_COLOR_BLACK);
    st7789_set_text_size(TEXT_SIZE);

    // Parte fixa
    st7789_draw_text_centered(120, 5, "Status Bateria", COLOR_LIGHT_PURPLE);
    st7789_draw_line(10, 25, 230, 25, COLOR_PURPLE);

    st7789_draw_text(10, 40, "Tensao:", COLOR_LIGHT_PURPLE);
    st7789_draw_text(10, 70, "Corrente:", COLOR_LIGHT_PURPLE);
    st7789_draw_text(10, 100, "VBUS:", COLOR_LIGHT_PURPLE);
    st7789_draw_text(10, 130, "Fonte:", COLOR_LIGHT_PURPLE);
    st7789_draw_text(10, 160, "Carga:", COLOR_LIGHT_PURPLE);

    st7789_draw_line(10, 190, 230, 190, COLOR_PURPLE);
    st7789_draw_text_centered(120, 220, "[BACK]", COLOR_LIGHT_PURPLE);

    
    
}
