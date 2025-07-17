
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "buzzer.h"
#include "spi_init.h"
#include "i2c_init.h"
#include "home.h"
#include "led_control.h"
#include "pin_def.h" 
#include "st7789.h"
#include "bq25896.h"
#include "driver/i2c.h"
#include "nvs_flash.h" 
#include "wifi_service.h" 



void kernel_init(void) {
  
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
 
    init_spi();
    init_i2c();
    buzzer_init();
    led_rgb_init();
    buzzer_boot_sequence();
    bq25896_init();
    ESP_ERROR_CHECK(ret);
    st7789_init(); 
    

   
    vTaskDelay(pdMS_TO_TICKS(1500));
}
