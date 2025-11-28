// Copyright (c) 2025 HIGH CODE LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "bluetooth_menu.h"
#include "bluetooth_service.h"
#include "apple_spam.h"
#include "icons.h"
#include "pin_def.h"
#include "driver/gpio.h"
#include "st7789.h"
#include "sub_menu.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "esp_log.h"
#include "bluetooth_scanner.h"
#include "rssi_analyser.h"

// --- DEFINIÇÕES DE CORES E LAYOUT ---
#define COLOR_BACKGROUND         ST7789_COLOR_BLACK
#define COLOR_TEXT_PRIMARY       ST7789_COLOR_WHITE
#define COLOR_TEXT_SECONDARY     ST7789_COLOR_GRAY
#define COLOR_HIGHLIGHT          ST7789_COLOR_PURPLE
#define COLOR_DIVIDER            0x4228

static BtDevice scanned_devices[20];
static int device_count = 0;
static const char *TAG = "BT_SCANNER";

// --- Protótipo da função auxiliar de desenho ---
static void draw_device_details_screen(const BtDevice *dev);

// --- Callback para eventos de GAP ---
static int gap_event_cb(struct ble_gap_event *event, void *arg) {
    if (event->type == BLE_GAP_EVENT_DISC) {
        if (device_count >= 20) return 0;
        for (int i = 0; i < device_count; i++) {
            if (ble_addr_cmp(&event->disc.addr, &scanned_devices[i].addr) == 0) {
                scanned_devices[i].rssi = event->disc.rssi;
                return 0;
            }
        }

        BtDevice *dev = &scanned_devices[device_count];
        *dev = (BtDevice){0}; // Limpa a estrutura
        dev->addr = event->disc.addr;
        dev->rssi = event->disc.rssi;
        dev->adv_type = event->disc.event_type;

        const uint8_t *data = event->disc.data;
        int len = event->disc.length_data;
        while (len > 1) {
            int field_len = data[0];
            if (field_len == 0 || field_len > len - 1) break;
            uint8_t type = data[1];
            if (type == BLE_HS_ADV_TYPE_COMP_NAME || type == BLE_HS_ADV_TYPE_INCOMP_NAME) {
                int name_len = field_len - 1;
                if (name_len >= sizeof(dev->name)) name_len = sizeof(dev->name) - 1;
                memcpy(dev->name, &data[2], name_len);
                dev->name[name_len] = '\0';
            } else if (type == BLE_HS_ADV_TYPE_MFG_DATA) {
                int mfg_len = field_len - 1;
                if (mfg_len > sizeof(dev->mfg_data)) mfg_len = sizeof(dev->mfg_data);
                memcpy(dev->mfg_data, &data[2], mfg_len);
                dev->mfg_data_len = mfg_len;
            } else if (type == BLE_HS_ADV_TYPE_SVC_DATA_UUID16) {
                if (field_len >= 3) {
                    dev->uuid16[0] = data[2];
                    dev->uuid16[1] = data[3];
                }
            }
            len -= (field_len + 1);
            data += (field_len + 1);
        }

        if (dev->name[0] == '\0') {
            snprintf(dev->name, sizeof(dev->name), "%02X:%02X:%02X:%02X:%02X:%02X",
                     dev->addr.val[5], dev->addr.val[4], dev->addr.val[3],
                     dev->addr.val[2], dev->addr.val[1], dev->addr.val[0]);
        }
        device_count++;
    } else if (event->type == BLE_GAP_EVENT_DISC_COMPLETE) {
        ESP_LOGI(TAG, "Scan completo, %d dispositivos encontrados", device_count);
    }
    return 0;
}

// --- PROTÓTIPOS DE FUNÇÕES ESTÁTICAS ---
static void show_device_details(int device_index);
static void bluetooth_action_scan(void);
static void bluetooth_action_advertise(void);
static void bluetooth_action_spam(void);

// Itens do Menu Bluetooth
static const SubMenuItem bluetoothMenuItems[] = {
    { "Scanner BLE", scan, bluetooth_action_scan },
    { "Advertise", blu_main, bluetooth_action_advertise },
    { "Spam BLE", blu_main, bluetooth_action_spam },
};
static const int bluetoothMenuSize = sizeof(bluetoothMenuItems) / sizeof(SubMenuItem);

void show_bluetooth_menu(void) {
    bluetooth_service_init();
    show_submenu(bluetoothMenuItems, bluetoothMenuSize, "Menu Bluetooth");
}

static void bluetooth_action_scan(void) {
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    st7789_set_text_size(2);
    st7789_draw_text_fb(15, 110, "Escaneando BLE...", ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    st7789_flush();

    struct ble_gap_disc_params scan_params = {0};
    scan_params.passive = 1;
    scan_params.filter_duplicates = 1;

    device_count = 0;
    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, 10000, &scan_params, gap_event_cb, NULL);
    vTaskDelay(pdMS_TO_TICKS(11000));

    if (device_count == 0) {
        st7789_fill_screen_fb(ST7789_COLOR_BLACK);
        st7789_draw_text_fb(15, 110, "Nenhum dispositivo!", ST7789_COLOR_RED, ST7789_COLOR_BLACK);
        st7789_flush();
        vTaskDelay(pdMS_TO_TICKS(2000));
        return;
    }

    SubMenuItem device_menu[device_count];
    char device_labels[device_count][33];
    for (int i = 0; i < device_count; i++) {
        strncpy(device_labels[i], scanned_devices[i].name, sizeof(device_labels[i]) - 1);
        device_labels[i][sizeof(device_labels[i]) - 1] = '\0';
        device_menu[i].label = device_labels[i];
        device_menu[i].icon = blu_main;
        device_menu[i].action = NULL;
    }

    int device_selection = 0;
    int device_offset = 0;
    bool stay_in_device_menu = true;

    while (stay_in_device_menu) {
        st7789_fill_screen_fb(ST7789_COLOR_BLACK);
        menu_draw_header("Dispositivos");
        if (device_selection < device_offset) {
            device_offset = device_selection;
        } else if (device_selection >= device_offset + MAX_VISIBLE_ITEMS) {
            device_offset = device_selection - MAX_VISIBLE_ITEMS + 1;
        }
        for (int i = 0; i < MAX_VISIBLE_ITEMS; i++) {
            int menuIndex = i + device_offset;
            if (menuIndex < device_count) {
                int posY = START_Y + i * (ITEM_HEIGHT + ITEM_SPACING);
                menu_draw_item(&device_menu[menuIndex], posY, menuIndex == device_selection);
            }
        }
        menu_draw_scrollbar(device_offset, device_count, device_selection);
        st7789_flush();

        bool input_processed = false;
        while (!input_processed) {
            if (!gpio_get_level(BTN_UP)) {
                while (!gpio_get_level(BTN_UP)) vTaskDelay(pdMS_TO_TICKS(200)); // <-- DELAY AJUSTADO
                device_selection = (device_selection - 1 + device_count) % device_count;
                input_processed = true;
            } else if (!gpio_get_level(BTN_DOWN)) {
                while (!gpio_get_level(BTN_DOWN)) vTaskDelay(pdMS_TO_TICKS(200)); // <-- DELAY AJUSTADO
                device_selection = (device_selection + 1) % device_count;
                input_processed = true;
            } else if (!gpio_get_level(BTN_OK)) {
                while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(200)); // <-- DELAY AJUSTADO
                show_device_details(device_selection);
                input_processed = true;
            } else if (!gpio_get_level(BTN_BACK)) {
                while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(200)); // <-- DELAY AJUSTADO
                stay_in_device_menu = false;
                input_processed = true;
            }
            vTaskDelay(pdMS_TO_TICKS(100)); // <-- DELAY DE POLLING AJUSTADO
        }
    }
}

// --- Função auxiliar para desenhar a tela de detalhes ---
static void draw_device_details_screen(const BtDevice *dev) {
    st7789_fill_screen_fb(COLOR_BACKGROUND);
    st7789_set_text_size(2);
    st7789_draw_text_fb(10, 10, "Detalhes do Dispositivo", COLOR_TEXT_PRIMARY, COLOR_BACKGROUND);
    st7789_draw_hline_fb(10, 35, 220, COLOR_DIVIDER);

    int y_pos = 45;
    st7789_set_text_size(1);

    char buffer[128]; // Buffer genérico para formatar strings
    // Nome
    st7789_draw_text_fb(10, y_pos, "Nome:", COLOR_TEXT_SECONDARY, COLOR_BACKGROUND);
    strncpy(buffer, dev->name, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    st7789_draw_text_fb(60, y_pos, buffer, COLOR_TEXT_PRIMARY, COLOR_BACKGROUND);
    y_pos += 20;

    // MAC
    snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
             dev->addr.val[5], dev->addr.val[4], dev->addr.val[3],
             dev->addr.val[2], dev->addr.val[1], dev->addr.val[0]);
    st7789_draw_text_fb(10, y_pos, "MAC:", COLOR_TEXT_SECONDARY, COLOR_BACKGROUND);
    st7789_draw_text_fb(60, y_pos, buffer, COLOR_TEXT_PRIMARY, COLOR_BACKGROUND);
    y_pos += 20;

    // RSSI
    snprintf(buffer, sizeof(buffer), "%d dBm", dev->rssi);
    st7789_draw_text_fb(10, y_pos, "RSSI:", COLOR_TEXT_SECONDARY, COLOR_BACKGROUND);
    st7789_draw_text_fb(60, y_pos, buffer, COLOR_TEXT_PRIMARY, COLOR_BACKGROUND);
    y_pos += 20;

    // Instruções
    st7789_draw_text_fb(10, 210, "RIGHT: Grafico RSSI", COLOR_HIGHLIGHT, COLOR_BACKGROUND);
    st7789_draw_text_fb(10, 225, "BACK: Voltar", COLOR_TEXT_SECONDARY, COLOR_BACKGROUND);
    st7789_flush();
}

// --- Função de detalhes do dispositivo ---
static void show_device_details(int device_index) {
    BtDevice *dev = &scanned_devices[device_index];
    bool stay_in_details = true;

    while (stay_in_details) {
        draw_device_details_screen(dev);

        bool input_processed = false;
        while (!input_processed) {
            if (!gpio_get_level(BTN_RIGHT)) {
                while (!gpio_get_level(BTN_RIGHT)) vTaskDelay(pdMS_TO_TICKS(200)); // <-- DELAY AJUSTADO
                show_rssi_analyser(dev);
                input_processed = true; // Força o redesenho da tela de detalhes ao voltar
            } else if (!gpio_get_level(BTN_BACK)) {
                while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(200)); // <-- DELAY AJUSTADO
                stay_in_details = false;
                input_processed = true;
            }
            vTaskDelay(pdMS_TO_TICKS(100)); // <-- DELAY DE POLLING AJUSTADO
        }
    }
}

// --- Funções de Advertise e Spam (com delays ajustados) ---
static void bluetooth_action_advertise(void) {
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    st7789_set_text_size(2);
    st7789_draw_text_fb(10, 80, "Anunciando como:", ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    st7789_draw_text_fb(10, 110, "HighBoy", COLOR_HIGHLIGHT, ST7789_COLOR_BLACK);
    st7789_set_text_size(1);
    st7789_draw_text_fb(10, 150, "Pressione BACK para parar", COLOR_TEXT_SECONDARY, ST7789_COLOR_BLACK);
    st7789_flush();
    bluetooth_service_start_advertising();
    while (gpio_get_level(BTN_BACK)) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(20));
    bluetooth_service_stop_advertising();
}

static void bluetooth_action_spam(void) {
    const int attack_count = spam_get_attack_count();
    if (attack_count <= 0) {
        st7789_fill_screen_fb(ST7789_COLOR_BLACK);
        st7789_draw_text_fb(10, 110, "Nenhum ataque", ST7789_COLOR_RED, ST7789_COLOR_BLACK);
        st7789_flush();
        vTaskDelay(pdMS_TO_TICKS(2000));
        return;
    }
    
    SubMenuItem attack_menu[attack_count];
    char attack_labels[attack_count][33];
    for (int i = 0; i < attack_count; i++) {
        const SpamType *attack_type = spam_get_attack_type(i);
        if (attack_type) {
            strncpy(attack_labels[i], attack_type->name, sizeof(attack_labels[i]) - 1);
            attack_labels[i][sizeof(attack_labels[i]) - 1] = '\0';
            attack_menu[i].label = attack_labels[i];
            attack_menu[i].icon = blu_main;
            attack_menu[i].action = NULL;
        }
    }

    int attack_selection = 0;
    int attack_offset = 0;
    bool stay_in_menu = true;

    while (stay_in_menu) {
        st7789_fill_screen_fb(ST7789_COLOR_BLACK);
        menu_draw_header("Selecione o Spam");
        if (attack_selection < attack_offset) {
            attack_offset = attack_selection;
        } else if (attack_selection >= attack_offset + MAX_VISIBLE_ITEMS) {
            attack_offset = attack_selection - MAX_VISIBLE_ITEMS + 1;
        }
        for (int i = 0; i < MAX_VISIBLE_ITEMS; i++) {
            int menuIndex = i + attack_offset;
            if (menuIndex < attack_count) {
                int posY = START_Y + i * (ITEM_HEIGHT + ITEM_SPACING);
                menu_draw_item(&attack_menu[menuIndex], posY, menuIndex == attack_selection);
            }
        }
        menu_draw_scrollbar(attack_offset, attack_count, attack_selection);
        st7789_flush();

        bool input_processed = false;
        while (!input_processed) {
            if (!gpio_get_level(BTN_UP)) {
                while (!gpio_get_level(BTN_UP)) vTaskDelay(pdMS_TO_TICKS(200)); // <-- DELAY AJUSTADO
                attack_selection = (attack_selection - 1 + attack_count) % attack_count;
                input_processed = true;
            } else if (!gpio_get_level(BTN_DOWN)) {
                while (!gpio_get_level(BTN_DOWN)) vTaskDelay(pdMS_TO_TICKS(200)); // <-- DELAY AJUSTADO
                attack_selection = (attack_selection + 1) % attack_count;
                input_processed = true;
            } else if (!gpio_get_level(BTN_OK)) {
                while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(200)); // <-- DELAY AJUSTADO
                const SpamType *selected_attack = spam_get_attack_type(attack_selection);
                st7789_fill_screen_fb(ST7789_COLOR_BLACK);
                st7789_draw_text_fb(10, 80, "Ativando Spam:", ST7789_COLOR_RED, ST7789_COLOR_BLACK);
                st7789_draw_text_fb(10, 110, selected_attack->name, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
                st7789_draw_text_fb(10, 150, "Pressione BACK para parar", COLOR_TEXT_SECONDARY, ST7789_COLOR_BLACK);
                st7789_flush();
                spam_start(attack_selection);
                while (gpio_get_level(BTN_BACK)) {
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                while(!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(20));
                spam_stop();
                input_processed = true;
                stay_in_menu = false; 
            } else if (!gpio_get_level(BTN_BACK)) {
                while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(200)); // <-- DELAY AJUSTADO
                stay_in_menu = false;
                input_processed = true;
            }
            vTaskDelay(pdMS_TO_TICKS(100)); // <-- DELAY DE POLLING AJUSTADO
        }
    }
}
