// Arquivo: led_control.c (VERSÃO CORRIGIDA E COMPLETA)

#include "led_control.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_RGB_GPIO 45 
static const char *TAG = "led_control";

// A variável do LED, acessível externamente
led_strip_handle_t led_strip;

// Inicializa o LED
void led_rgb_init(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_RGB_GPIO,
        .max_leds = 1,
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_ERROR_CHECK(led_strip_clear(led_strip)); 
    ESP_LOGI(TAG, "LED RGB inicializado no GPIO %d", LED_RGB_GPIO);
}

// Define uma cor estática no LED
void led_set_color(uint8_t r, uint8_t g, uint8_t b) {
    if (led_strip) {
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
    }
}

// Pisca o LED com uma cor e duração específicas
static void led_blink_color(uint8_t r, uint8_t g, uint8_t b, int duration_ms) {
    led_set_color(r, g, b);
    vTaskDelay(duration_ms / portTICK_PERIOD_MS);
    if (led_strip) {
        ESP_ERROR_CHECK(led_strip_clear(led_strip));
    }
}

// --- CÓDIGO FALTANDO ADICIONADO AQUI ---

void led_blink_red(void) {
    led_blink_color(255, 0, 0, 500);
}

void led_blink_green(void) {
    led_blink_color(0, 255, 0, 500);
}

void led_blink_blue(void) {
    led_blink_color(0, 0, 255, 500);
}

void led_blink_purple(void) {
    led_blink_color(128, 0, 128, 500);
}