#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "led_strip.h"
#include "led_strip_rmt.h"
#include "st7789.h"
#include "home.h"
#include "buzzer.h"
#include "menu.h"
#include "kernel.h"


void app_main(void) {

    kernel_init();

    xTaskCreate(menu_task, "menu_task", 4096, NULL, 5, NULL);
}
