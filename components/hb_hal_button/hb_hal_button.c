// hb_hal_button.c
#include "hb_hal_button.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Configuração dos botões (ajuste os pinos aqui!)
static const struct {
    gpio_num_t gpio;
    bool active_low;       // true se o botão conecta ao GND quando pressionado
    uint32_t hold_time_ms; // Tempo para considerar "segurando" (ex: 1000ms)
} button_config[HB_BUTTON_COUNT] = {
    [HB_BUTTON_UP]    = { .gpio = GPIO_NUM_15, .active_low = true, .hold_time_ms = 1000 },
    [HB_BUTTON_DOWN]  = { .gpio = GPIO_NUM_6, .active_low = true, .hold_time_ms = 1000 },
    [HB_BUTTON_OK]    = { .gpio = GPIO_NUM_4, .active_low = true, .hold_time_ms = 1000 },
    [HB_BUTTON_BACK]  = { .gpio = GPIO_NUM_7, .active_low = true, .hold_time_ms = 1000 },
    [HB_BUTTON_LEFT]  = { .gpio = GPIO_NUM_5, .active_low = true, .hold_time_ms = 1000 },
    [HB_BUTTON_RIGHT] = { .gpio = GPIO_NUM_16, .active_low = true, .hold_time_ms = 1000 }
};

// Estados internos
static hb_button_state_t button_states[HB_BUTTON_COUNT];
static uint32_t last_press_time[HB_BUTTON_COUNT];
static const uint32_t debounce_delay_ms = 50;

void hb_button_init(void) {
    for (int i = 0; i < HB_BUTTON_COUNT; i++) {
        // Configura GPIO
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << button_config[i].gpio),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = button_config[i].active_low ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
            .pull_down_en = button_config[i].active_low ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);

        button_states[i] = HB_BUTTON_STATE_RELEASED;
        last_press_time[i] = 0;
    }
}

void hb_button_loop(void) {
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

    for (int i = 0; i < HB_BUTTON_COUNT; i++) {
        bool raw_state = gpio_get_level(button_config[i].gpio);
        if (button_config[i].active_low) raw_state = !raw_state;

        // Debounce
        if (raw_state != (button_states[i] != HB_BUTTON_STATE_RELEASED)) {
            if ((current_time - last_press_time[i]) > debounce_delay_ms) {
                if (raw_state) {
                    button_states[i] = HB_BUTTON_STATE_PRESSED;
                    last_press_time[i] = current_time;
                } else {
                    button_states[i] = HB_BUTTON_STATE_RELEASED;
                }
            }
        }

        // Verifica se está sendo segurado
        if (button_states[i] == HB_BUTTON_STATE_PRESSED && 
            (current_time - last_press_time[i]) > button_config[i].hold_time_ms) {
            button_states[i] = HB_BUTTON_STATE_HELD;
        }
    }

    vTaskDelay(10 / portTICK_PERIOD_MS); // Intervalo de atualização
}

bool hb_button_is_pressed(hb_button_t button) {
    if (button >= HB_BUTTON_COUNT) return false;
    bool pressed = (button_states[button] == HB_BUTTON_STATE_PRESSED);
    if (pressed) button_states[button] = HB_BUTTON_STATE_HELD; // Reseta o estado
    return pressed;
}

bool hb_button_is_held(hb_button_t button) {
    if (button >= HB_BUTTON_COUNT) return false;
    return (button_states[button] == HB_BUTTON_STATE_HELD);
}