#ifndef HB_HAL_I2C_H
#define HB_HAL_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Inicializa o I2C e realiza a varredura dos dispositivos.
 *
 * Configura o I2C no modo master utilizando os pinos e a velocidade de clock
 * especificados, varre o intervalo de endereços válidos (0x03 a 0x77)
 * e registra via log todos os dispositivos que respondem.
 *
 * @return void
 */
void i2c_scan(void);

#ifdef __cplusplus
}
#endif

#endif
