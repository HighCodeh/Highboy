
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "buzzer.h"
#include "display.h"
#include "home.h"
#include "led_control.h"
#include "pin_def.h" 
#include "st7789.h"
#include "bq25896.h"
#include "driver/i2c.h"
#include "nvs_flash.h" // Adicionar este include
#include "wifi_service.h" 






void kernel_init(void) {
    // --- ETAPA 1: Inicializar periféricos simples ---
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
 
    spi_bus_config_t buscfg = {
        .mosi_io_num = 11,
        .miso_io_num = 13, // MISO é necessário para o SD Card
        .sclk_io_num = 12,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32768,  // Tamanho de transferência genérico
    };
    ret = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 8,
        .scl_io_num = 9,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    
    buzzer_init();
    led_rgb_init();
    buzzer_boot_sequence();
    bq25896_attach_to_bus();
    ESP_ERROR_CHECK(ret);
    st7789_init(); 
    

   
    vTaskDelay(pdMS_TO_TICKS(1500));
}
