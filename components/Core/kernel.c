
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
#include "i2c_init.h"
#include "bq25896.h"

typedef enum {
    STATE_HOME,
    STATE_MENU
} app_state_t;

static app_state_t current_state = STATE_HOME;
// === ESTADOS DA INTERFACE ===
void kernel_init(void){
    i2c_master_init();
    bq25896_init();
    buzzer_init();
    buzzer_boot_sequence(); 
    led_rgb_init();
    init_display();

}
