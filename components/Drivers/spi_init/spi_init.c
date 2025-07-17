#include <stdio.h>
#include "driver/spi_master.h"
#include "esp_log.h"
#include "st7789.h"
#include "pin_def.h"

#define TAG "SPIInit"

void init_spi(void) {
    spi_bus_config_t buscfg = {
        .mosi_io_num = ST7789_PIN_MOSI,
        .sclk_io_num = ST7789_PIN_SCLK,
        .miso_io_num = -1,        
        .quadwp_io_num = -1,      
        .quadhd_io_num = -1,      
        .max_transfer_sz = 32768, 
    };

    esp_err_t ret = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao inicializar SPI: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Barramento SPI inicializado com sucesso.");
    }
}
