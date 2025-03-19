// hb_hal_gpio.h
#pragma once
#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_err.h"

// Tipos de modo do GPIO
typedef enum {
    HB_GPIO_MODE_INPUT,
    HB_GPIO_MODE_OUTPUT
} hb_gpio_mode_t;

// Inicializa um pino
esp_err_t hb_gpio_init(gpio_num_t pin, hb_gpio_mode_t mode);

// Escreve no pino (saída)
void hb_gpio_write(gpio_num_t pin, bool state);

// Lê o pino (entrada)
bool hb_gpio_read(gpio_num_t pin);