#include "driver/i2c.h"
#include "esp_log.h"

#define TAG "I2CInit"

void init_i2c(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 8,
        .scl_io_num = 9,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000, // 400 kHz
    };

    esp_err_t ret = i2c_param_config(I2C_NUM_0, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro em i2c_param_config: %s", esp_err_to_name(ret));
        return;
    }

    ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro em i2c_driver_install: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "I2C mestre inicializado com sucesso no I2C_NUM_0.");
}
