#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "I2C_SCANNER";

void i2c_scan(void)
{
    // Configuração do barramento I2C
    i2c_master_bus_handle_t i2c_bus_handle;
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 9,  // Ajuste conforme seu hardware
        .sda_io_num = 8,  // Ajuste conforme seu hardware
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao criar o barramento I2C: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "I2C inicializado na porta %d", bus_config.i2c_port);
    ESP_LOGI(TAG, "Iniciando varredura de dispositivos I2C...");

    uint8_t devices_found = 0;
    for (uint8_t addr = 3; addr < 0x78; addr++) {
        ret = i2c_master_probe(i2c_bus_handle, addr, 1000);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Dispositivo encontrado no endereço: 0x%02X", addr);
            devices_found++;
        }
    }

    if (devices_found == 0) {
        ESP_LOGI(TAG, "Nenhum dispositivo I2C encontrado.");
    } else {
        ESP_LOGI(TAG, "Varredura completa: %d dispositivo(s) encontrado(s).", devices_found);
    }

    // Remover o barramento I2C
    i2c_del_master_bus(i2c_bus_handle);
}
