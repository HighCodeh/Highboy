
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "buzzer.h"
#include "display.h"
#include "ir_transmiter.h"
#include "home.h"
#include "led_control.h"
#include "pin_def.h" 

typedef enum {
    STATE_HOME,
    STATE_MENU
} app_state_t;

static app_state_t current_state = STATE_HOME;
// === ESTADOS DA INTERFACE ===
void app_main(void) {
}

void kernel_init(void){

    ir_init();

    buzzer_init();
    buzzer_boot_sequence(); // Beep de boot (bonito e correto)

    led_rgb_init();
    init_display();

    // xTaskCreate(menu_task, "menu_task", 4096, NULL, 5, NULL); //adicionar depois no main
}
