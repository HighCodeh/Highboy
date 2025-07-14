#include "menu_generic.h"
#include "st7789.h"
#include "pin_def.h"
#include "driver/gpio.h"
#include "led_control.h"
#include "home.h"
#include "wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "icons.h"
#include "brightness_ui.h"
#include "led_ui.h"
#include "info_device.h"
#include "buzzer_ui.h"
#include "battery_ui.h"
#include "bq25896.h"
#include "sub_menu.h"
typedef enum {
    STATE_HOME,
    STATE_MENU,
    STATE_CONFIG,
    STATE_INFO_DEVICE
   
} app_state_t;

static app_state_t current_state = STATE_HOME;

//Exemplo Sub_menus
//void show_ir_submenu(void) {
// Array de itens para o submenu de IR
//    const SubMenuItem ir_items[] = {
//        { "Label",       Icone,  Ação Final},
//        { "Universal",   wifi_main,   NULL },
//        { "Aprender",    wifi_main,   NULL },
//    };
//    int item_count = sizeof(ir_items) / sizeof(ir_items[0]);
// Chama a função da biblioteca para exibir este submenu
//    show_submenu(ir_items, item_count, " IR");
//}


MenuItem main_menu_items[] = {
    {"WiFi", wifi_main, show_wifi_submenu},
    {"Bluetooth", blu_main, NULL},
    {"NFC", nfc_main, NULL},
    {"RF", rf_main, NULL},
    {"Infravermelho", infra_main, NULL},
    {"BadUSB", bad, NULL},
    {"GPIOS", conf_main, NULL},
    {"MicroSD", sd_main, NULL},
    
};

MenuItem config_menu_items[] = {
    {"Brilho", brilho, show_brightness_screen}, 
    {"bluetooth", blu_main, NULL},
    {"Buzzer", Music, show_buzzer_screen},
    {"Tela", inatividade, NULL},
    {"Led", led_main, show_led_screen},
    {"Bateria", bat_main, show_battery_screen},
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
        .pin_bit_mask = (1ULL << BTN_UP) | (1ULL << BTN_DOWN) | (1ULL << BTN_OK) | (1ULL << BTN_BACK) | (1ULL << BTN_LEFT) | (1ULL << BTN_RIGHT),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);

    // --- NOVO: Inicializa o chip da bateria ---
    if (bq25896_init() == ESP_OK) {
        printf("BQ25896 (bateria) inicializado com sucesso!\n");
    } else {
        printf("Falha ao inicializar o BQ25896!\n");
    }
    // ------------------------------------------

    current_state = STATE_HOME;
    while (1) {
        switch (current_state) {
            case STATE_HOME:
                // --- LÓGICA DA TELA HOME ATUALIZADA ---

                // 1. Lê os dados da bateria
                uint16_t voltage = bq25896_get_battery_voltage();
                int percentage = bq25896_get_battery_percentage(voltage);
                bool is_charging = bq25896_is_charging();
                
                home(is_charging, percentage);

                for (int i = 0; i < 100; i++) { 
                    if (!gpio_get_level(BTN_LEFT)) {
                        current_state = STATE_MENU;
                        vTaskDelay(pdMS_TO_TICKS(50));
                        break;
                    } else if (!gpio_get_level(BTN_DOWN)) {
                        current_state = STATE_CONFIG;
                        vTaskDelay(pdMS_TO_TICKS(50));
                        break;
                    } else if (!gpio_get_level(BTN_RIGHT)) {
                        current_state = STATE_INFO_DEVICE;
                        vTaskDelay(pdMS_TO_TICKS(50));
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                
                break;

            case STATE_MENU:
                show_menu(&menu_main);
                current_state = STATE_HOME;
                break;

            case STATE_CONFIG:
                show_menu(&Config_main);
                current_state = STATE_HOME;
                break;
            
            case STATE_INFO_DEVICE:
                show_device_info_screen();
                current_state = STATE_HOME;
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}