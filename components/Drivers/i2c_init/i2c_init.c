#include <stdio.h>
#include "i2c_init.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "pin_def.h"

static const char *TAG = "I2C_INIT";

/**
 * @brief Inicializa o controlador I2C do ESP32 em modo mestre.
 */
// A CORREÇÃO ESTÁ AQUI: A função deve ter apenas um tipo de retorno.
// O correto é 'esp_err_t', pois ela retorna códigos de erro.
esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar I2C: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao instalar driver I2C: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "Driver I2C inicializado com sucesso.");
    return ESP_OK;
}