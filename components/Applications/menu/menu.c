#include "menu_generic.h"
#include "st7789.h"
#include "pin_def.h"
#include "driver/gpio.h"
#include "led_control.h"
#include "home.h"
#include "wifi.h"
#include "bad_usb_menu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "icons.h"
#include "brightness_ui.h"
#include "battery_ui.h"
#include "GPIO.h"


typedef enum {
    STATE_HOME,
    STATE_MENU,
    STATE_CONFIG
   
} app_state_t;

static app_state_t current_state = STATE_HOME;


MenuItem main_menu_items[] = {
    {"WiFi", wifi_main, show_wifi_menu},
    {"Bluetooth", blu_main, NULL},
    {"NFC", nfc_main, NULL},
    {"RF", rf_main, NULL},
    {"Infravermelho", infra_main, NULL},
    {"BadUSB", bad, show_bad_usb_menu},
    {"GPIOS", conf_main, show_gpio_menu},
    {"MicroSD", sd_main, NULL},
};

MenuItem config_menu_items[] = {
    {"Brilho", brilho, show_brightness_screen},
    {"bluetooth", blu_main, NULL},
    {"Buzzer", Music, NULL},
    {"Tela", inatividade, NULL},
    {"Bateria", NULL, show_battery_screen},
};

Menu menu_main = {
    .title = "Menu Principal",
    .items = main_menu_items,
    .item_count = sizeof(main_menu_items) / sizeof(main_menu_items[0])
};

Menu Config_main = {
    .title = "Menu Configuracoes",
    .items = config_menu_items,
    .item_count = sizeof(config_menu_items) / sizeof(config_menu_items[0])
};


// Task da interface
void menu_task(void *pvParameters) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BTN_UP) | (1ULL << BTN_DOWN) | (1ULL << BTN_OK) | (1ULL << BTN_BACK) | (1ULL << BTN_LEFT),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);

    current_state = STATE_HOME;

    while (1) {
        switch (current_state) {
            case STATE_HOME:
                home();
                led_blink_blue();
                while (current_state == STATE_HOME) {
                    if (!gpio_get_level(BTN_LEFT)) {
                        current_state = STATE_MENU;
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        break;
                    } else if (!gpio_get_level(BTN_DOWN)) {
                        current_state = STATE_CONFIG;
                        vTaskDelay(pdMS_TO_TICKS(50));
                        break;
                    }

                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                break;

            case STATE_MENU:
                led_blink_purple();
                show_menu(&menu_main);
                current_state = STATE_HOME;
                break;

            case STATE_CONFIG:
                led_blink_green();
                show_menu(&Config_main);
                current_state = STATE_HOME;
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

 
