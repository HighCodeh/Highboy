#include "backlight.h"
#include "st7789.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_RESOLUTION         LEDC_TIMER_8_BIT
#define LEDC_FREQUENCY          5000 // 5 kHz

static uint8_t current_brightness = 255;

void backlight_init(void) {
    ledc_timer_config_t timer_conf = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_RESOLUTION,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) {
        ESP_LOGE("BACKLIGHT", "Erro ao configurar timer LEDC: %d", err);
    }

    ledc_channel_config_t channel_conf = {
        .gpio_num       = ST7789_PIN_BL,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .duty           = current_brightness,
        .hpoint         = 0
    };
    err = ledc_channel_config(&channel_conf);
    if (err != ESP_OK) {
        ESP_LOGE("BACKLIGHT", "Erro ao configurar canal LEDC: %d", err);
    }

    backlight_set_brightness(current_brightness);
}

void backlight_set_brightness(uint8_t brightness) {
    // A verificação 'if (brightness > 255)' foi removida, pois é sempre falsa para um uint8_t.
    // A lógica de limitação já é feita na UI.
    current_brightness = brightness;
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, brightness);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

uint8_t backlight_get_brightness(void) {
    return current_brightness;
}