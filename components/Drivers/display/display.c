#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/spi_master.h"
#include "st7789.h"
#include "pin_def.h" 
#include "esp_log.h"

void init_display(void) {
    spi_bus_config_t buscfg = {
        .mosi_io_num = ST7789_PIN_MOSI,
        .sclk_io_num = ST7789_PIN_SCLK,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        // ✨ ALTERADO: Aumenta o tamanho máximo para a transferência do framebuffer inteiro
        .max_transfer_sz = ST7789_WIDTH * ST7789_HEIGHT * 2 + 8,
    };

    esp_err_t ret = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        printf("Erro ao inicializar SPI: %s\n", esp_err_to_name(ret));
        return;
    }



    st7789_set_text_size(2);
}
