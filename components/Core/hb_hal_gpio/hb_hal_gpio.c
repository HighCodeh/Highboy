
#include "hb_hal_gpio.h"

esp_err_t hb_gpio_init(gpio_num_t pin, hb_gpio_mode_t mode) {
    // Configura direção do pino
    gpio_mode_t esp_mode = (mode == HB_GPIO_MODE_INPUT) ? GPIO_MODE_INPUT : GPIO_MODE_OUTPUT;
    return gpio_set_direction(pin, esp_mode);
}

void hb_gpio_write(gpio_num_t pin, bool state) {
    gpio_set_level(pin, state);
}

bool hb_gpio_read(gpio_num_t pin) {
    return (bool)gpio_get_level(pin);
}