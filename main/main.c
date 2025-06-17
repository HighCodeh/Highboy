#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "led_strip.h"
#include "led_strip_rmt.h"
#include "st7789.h"
#include "home.h"
#include "buzzer.h"
#include "menu.h"
#include "ir_transmitter.h"

// === DEFINIÇÕES DE PINOS ===
#define BTN_LEFT     5
#define BTN_BACK     7
#define LED_GPIO     45
#define LED_COUNT    1

// === ESTADOS DA INTERFACE ===
typedef enum {
    STATE_HOME,
    STATE_MENU
} app_state_t;

static app_state_t current_state = STATE_HOME;
static led_strip_handle_t led_strip;


typedef struct {
    int freq;
    int duration_ms;
} Note;

static const Note pop_music[] = {
    {440, 120}, {0, 30}, {880, 120}, {0, 30},
    {660, 100}, {0, 30}, {990, 100}, {0, 30},
    {550, 100}, {0, 20}, {1100, 130}, {0, 50},
    {880, 100}, {0, 30}, {1320, 150}, {0, 40},
    {330, 80}, {0, 20}, {330, 80}, {0, 20},
    {330, 80}, {0, 40}, {660, 150}, {0, 30},
    {550, 120}, {0, 30}, {880, 80}, {660, 80},
    {990, 80}, {660, 80}, {880, 150}, {0, 50},
    {1100, 100}, {0, 40}, {1320, 200}, {0, 80}
};

void play_pop_music(void) {
    int total_notes = sizeof(pop_music) / sizeof(Note);
    for (int i = 0; i < total_notes; i++) {
        buzzer_play_tone(pop_music[i].freq, pop_music[i].duration_ms);
    }
}
// === INICIALIZA LED WS2812 COM COR ROXO ===
void init_led_rgb(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_COUNT,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags.with_dma = false,
    };

    led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    led_strip_clear(led_strip);

    // Cor roxo intenso
    led_strip_set_pixel(led_strip, 0, 150, 0, 220);
    led_strip_refresh(led_strip);
}

// === INICIALIZA DISPLAY E SPI ===
void init_display(void) {
    spi_bus_config_t buscfg = {
        .mosi_io_num = ST7789_PIN_MOSI,
        .sclk_io_num = ST7789_PIN_SCLK,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = ST7789_MAX_CHUNK_BYTES,
    };

    esp_err_t ret = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        printf("Erro ao inicializar SPI: %s\n", esp_err_to_name(ret));
        return;
    }

    st7789_init();
    st7789_enable_framebuffer();
    st7789_set_backlight(true);
    st7789_fill_screen(ST7789_COLOR_BLACK);
    st7789_set_text_size(2);
}

// === TAREFA PRINCIPAL DO MENU ===
void menu_task(void *pvParameters) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BTN_LEFT) | (1ULL << BTN_BACK),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);

    menu_init();

    while (1) {
        switch (current_state) {
            case STATE_HOME:
                home();
                while (1) {
                    if (!gpio_get_level(BTN_LEFT)) {
                        current_state = STATE_MENU;
                        vTaskDelay(pdMS_TO_TICKS(200));
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                break;

            case STATE_MENU:
                showMenu();
                handleMenuControls();
                current_state = STATE_HOME;
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// === FUNÇÃO PRINCIPAL ===
void app_main(void) {
    ir_init();

    buzzer_init();
    buzzer_boot_sequence(); // Beep de boot (bonito e correto)

    init_led_rgb();
    init_display();

    xTaskCreate(menu_task, "menu_task", 4096, NULL, 5, NULL);
}
