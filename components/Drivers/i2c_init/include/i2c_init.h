// i2c_init.h

#ifndef I2C_INIT_H
#define I2C_INIT_H

#include "driver/i2c.h" // Incluir dependências necessárias para o protótipo

/**
 * @brief Inicializa o controlador I2C do ESP32 em modo mestre.
 * * @return 
 * - ESP_OK: Sucesso na inicialização.
 * - Outros valores: Falha na inicialização (ver esp_err_t).
 */
esp_err_t i2c_master_init(void);


#endif // Fim do I2C_INIT_H