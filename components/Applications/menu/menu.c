#include "menu_generic.h"
#include "st7789.h"
#include "pin_def.h"
#include "driver/gpio.h"
#include "led_control.h"
#include "home.h"
#include "wifi.h"
#include "infrared.h"
#include "bluetooth_menu.h"
#include "bad_usb_menu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "icons.h"
#include "brightness_ui.h"
#include "battery_ui.h"
#include "GPIO.h"
#include "storage_api.h" // <-- GARANTA QUE ESTE INCLUDE ESTÁ CORRETO


#define COLOR_BLACK       ST7789_COLOR_BLACK
#define COLOR_WHITE       ST7789_COLOR_WHITE
#define COLOR_RED         ST7789_COLOR_RED
#define COLOR_GREEN       ST7789_COLOR_GREEN
#define COLOR_BLUE        ST7789_COLOR_BLUE
#define COLOR_YELLOW      ST7789_COLOR_YELLOW
#define COLOR_GRAY        ST7789_COLOR_GRAY

// Protótipo da nova função
void show_sd_menu(void);


typedef enum {
    STATE_HOME,
    STATE_MENU,
    STATE_CONFIG
   
} app_state_t;

static app_state_t current_state = STATE_HOME;


MenuItem main_menu_items[] = {
    {"WiFi", wifi_main, show_wifi_menu},
    {"Bluetooth", blu_main, show_bluetooth_menu},
    {"NFC", nfc_main, NULL},
    {"RF", rf_main, NULL},
    {"Infravermelho", infra_main, show_infrared_menu},
    {"BadUSB", bad, show_bad_usb_menu},
    {"GPIOS", conf_main, show_gpio_menu},
    {"MicroSD", sd_main, show_sd_menu},
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

void draw_file_list(const file_list_t *list, int selected_index) {
    st7789_fill_screen_fb(COLOR_BLACK);
    st7789_draw_text_fb(5, 5, "MicroSD", COLOR_WHITE, COLOR_BLACK);
    st7789_draw_hline_fb(0, 25, 240, COLOR_GRAY);

    if (list->count == 0) {
        st7789_draw_text_fb(20, 100, "Nenhum arquivo encontrado", COLOR_YELLOW, COLOR_BLACK);
    } else {
        for (int i = 0; i < list->count; i++) {
            uint16_t text_color = (i == selected_index) ? COLOR_BLACK : COLOR_WHITE;
            uint16_t bg_color = (i == selected_index) ? COLOR_WHITE : COLOR_BLACK;
            
            char display_name[35];

            // --- INÍCIO DA CORREÇÃO ---
            const char* prefix = list->is_dir[i] ? "[D] " : "    ";
            const char* filename = list->names[i];

            // A mágica está no "%.28s". Isso diz ao snprintf:
            // "Pegue a string 'filename', mas use no máximo 28 caracteres dela."
            // Isso garante que 4 (prefixo) + 28 (nome) + 1 (null) cabem nos 35 bytes.
            snprintf(display_name, sizeof(display_name), "%s%.28s", prefix, filename);
            // --- FIM DA CORREÇÃO ---
            
            st7789_draw_text_fb(10, 35 + (i * 15), display_name, text_color, bg_color);
        }
    }
    st7789_flush();
}

void show_file_content_screen(const char* filename, const char* content) {
    st7789_fill_screen_fb(COLOR_BLACK);
    st7789_draw_text_fb(5, 5, filename, COLOR_YELLOW, COLOR_BLACK);
    st7789_draw_hline_fb(0, 25, 240, COLOR_GRAY);

    // Exibe o conteúdo do arquivo
    st7789_draw_text_fb(5, 35, content, COLOR_WHITE, COLOR_BLACK);

    st7789_flush();

    // Espera o usuário pressionar "Voltar"
    vTaskDelay(pdMS_TO_TICKS(500));
    while (gpio_get_level(BTN_BACK)) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void show_sd_menu(void) {
    // Aloca memória para a lista na HEAP para segurança
    file_list_t* file_list = malloc(sizeof(file_list_t));
    if (file_list == NULL) return;

    int selected_item = 0;

    if (storage_list_files("/", file_list) != ESP_OK) {
        st7789_fill_screen_fb(COLOR_BLACK);
        st7789_draw_text_fb(20, 100, "Erro ao ler o cartao SD!", COLOR_RED, COLOR_BLACK);
        st7789_flush();
        vTaskDelay(pdMS_TO_TICKS(2000));
        free(file_list);
        return;
    }

    while (1) {
        draw_file_list(file_list, selected_item);

        if (!gpio_get_level(BTN_DOWN)) {
            selected_item = (selected_item + 1) % (file_list->count == 0 ? 1 : file_list->count);
            vTaskDelay(pdMS_TO_TICKS(200));
        } else if (!gpio_get_level(BTN_UP)) {
            selected_item = (selected_item - 1 + (file_list->count == 0 ? 1 : file_list->count)) % (file_list->count == 0 ? 1 : file_list->count);
            vTaskDelay(pdMS_TO_TICKS(200));
        } else if (!gpio_get_level(BTN_OK)) {
            if (file_list->count > 0 && !file_list->is_dir[selected_item]) {
                char* file_content = malloc(2048);
                if (file_content) {
                    size_t bytes_read;
                    char file_path[256];
                    snprintf(file_path, sizeof(file_path), "/%s", file_list->names[selected_item]);

                    if (storage_read_file(file_path, file_content, 2048, &bytes_read) == ESP_OK) {
                        show_file_content_screen(file_list->names[selected_item], file_content);
                    } else {
                        st7789_fill_rect_fb(0, 100, 240, 40, COLOR_RED);
                        st7789_draw_text_fb(10, 115, "Erro ao ler arquivo!", COLOR_WHITE, COLOR_RED);
                        st7789_flush();
                        vTaskDelay(pdMS_TO_TICKS(1500));
                    }
                    free(file_content);
                }
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        } else if (!gpio_get_level(BTN_BACK)) {
            vTaskDelay(pdMS_TO_TICKS(200));
            break; 
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    free(file_list);
}

 
