#include "wifi.h"
#include "driver/gpio.h"
#include "esp_log.h" // Certifique-se que este arquivo contém o ícone 'wifi_main'
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "icons.h"
#include "led_control.h"
#include "pin_def.h"
#include "st7789.h"
#include "sub_menu.h"
#include "wifi_deauther.h"
#include "wifi_service.h"
#include "evil_twin.h"
#include <string.h>
#include "sub_menu.h"
// =================================================================
// Definições e Variáveis Globais
// =================================================================

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





static const char *TAG = "wifi";
static SemaphoreHandle_t wifi_mutex = NULL;


// =================================================================
// Protótipos das Funções
// =================================================================
static void wifi_action_scan(void);
static void wifi_action_attack(void);



static void draw_header_fb(const char *title);

static void draw_menu_item_fb(const SubMenuItem *item, int y, bool selected);

// =================================================================
// Itens do Menu WiFi
// =================================================================


// (As funções de ataque como 'get_deauth_frame_template' e 'wifi_deauther_send_deauth_frame' permanecem as mesmas)
// ... (código de ataque omitido por brevidade, mas deve ser mantido no seu arquivo) ...


// =================================================================
// Ações dos Itens do Menu
// =================================================================

// Ação para "Scannear Rede"
static void wifi_action_scan(void) {
    wifi_init();
    wifi_service_init();
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    st7789_set_text_size(2);
    st7789_draw_text_fb(20, 110, "Escaneando redes...", ST7789_COLOR_PURPLE, ST7789_COLOR_BLACK);
    st7789_flush();
    
    wifi_deauther_scan(); // Executa o scan
    
    vTaskDelay(pdMS_TO_TICKS(500)); // Pequeno delay para mostrar a mensagem
}

// Ação para "Atacar Alvo" (agora um submenu)
// Arquivo: wifi.c

// Ação para "Atacar Alvo" com o fluxo de retorno corrigido
static void wifi_action_attack(void) {
    while (true) { // <-- NOVO: Loop principal para manter o usuário nesta seção
        // 1. Verificar se existem redes escaneadas
        if (get_stored_ap_count() == 0) {
            st7789_fill_screen_fb(ST7789_COLOR_BLACK);
            st7789_draw_text_fb(15, 110, "Nenhuma rede escaneada!", ST7789_COLOR_RED, ST7789_COLOR_BLACK);
            st7789_flush();
            vTaskDelay(pdMS_TO_TICKS(2000));
            return; // Se não há redes, sai da função
        }

        // 2. Preparar os itens do menu a partir das redes encontradas
        int ap_count = get_stored_ap_count();
        SubMenuItem ap_menu_items[ap_count];
        char ap_labels[ap_count][33];

        for (int i = 0; i < ap_count; i++) {
            const wifi_ap_record_t* ap = get_stored_ap_record(i);
            strncpy(ap_labels[i], (const char*)ap->ssid, 32);
            ap_labels[i][32] = '\0';
            
            ap_menu_items[i].label = ap_labels[i];
            ap_menu_items[i].icon = wifi_main;
            ap_menu_items[i].action = NULL;
        }

        // 3. Chamar o menu de seleção
        int selected_index = show_selection_menu(ap_menu_items, ap_count, "Selecionar Alvo");

        // 4. Agir com base na seleção
        if (selected_index == -1) {
            break; // <-- NOVO: Se o usuário apertar "Voltar" na lista, sai do loop e da função
        }
        
        // Se um alvo foi selecionado, mostra a tela de ataque
        const wifi_ap_record_t* target = get_stored_ap_record(selected_index);
        
        st7789_fill_screen_fb(ST7789_COLOR_BLACK);
        char attack_msg[64];
        snprintf(attack_msg, sizeof(attack_msg), "Atacando %s...", target->ssid);
        st7789_set_text_size(2);
        st7789_draw_text_fb(20, 110, attack_msg, ST7789_COLOR_RED, ST7789_COLOR_BLACK);
        st7789_draw_text_fb(20, 140, "Pressione BACK", ST7789_COLOR_GRAY, ST7789_COLOR_BLACK);
        st7789_flush();
        
        // Loop de ataque (só sai quando o usuário aperta BACK)
        while (gpio_get_level(BTN_BACK)) {
            wifi_deauther_send_deauth_frame(target, DEAUTH_INVALID_AUTH);
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        while(!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(20));

        // <-- NOVO: Ao sair do loop de ataque, o loop `while(true)` recomeçará,
        // mostrando a lista de seleção de redes novamente.
    }
}

// Ação para "Evil Twin" com o fluxo de retorno corrigido
static void wifi_action_evil_twin(void) {
    while (true) { // <-- NOVO: Loop principal
        // 1. Verificar redes
        if (get_stored_ap_count() == 0) {
            st7789_fill_screen_fb(ST7789_COLOR_BLACK);
            st7789_draw_text_fb(15, 110, "Nenhuma rede escaneada!", ST7789_COLOR_RED, ST7789_COLOR_BLACK);
            st7789_flush();
            vTaskDelay(pdMS_TO_TICKS(2000));
            return;
        }

        // 2. Preparar menu de redes
        int ap_count = get_stored_ap_count();
        SubMenuItem ap_menu_items[ap_count];
        char ap_labels[ap_count][33];

        for (int i = 0; i < ap_count; i++) {
            const wifi_ap_record_t* ap = get_stored_ap_record(i);
            strncpy(ap_labels[i], (const char*)ap->ssid, 32);
            ap_labels[i][32] = '\0';

            ap_menu_items[i].label = ap_labels[i];
            ap_menu_items[i].icon = portal;
            ap_menu_items[i].action = NULL;
        }

        // 3. Chamar menu de seleção
        int selected_index = show_selection_menu(ap_menu_items, ap_count, "Alvo para Evil Twin");

        // 4. Agir com base na seleção
        if (selected_index == -1) {
            break; // <-- NOVO: Sai do loop e volta para o menu Wi-Fi
        }
        
        // Se um alvo foi selecionado, inicia o ataque
        const wifi_ap_record_t* target = get_stored_ap_record(selected_index);
        
        evil_twin_start_attack((const char*)target->ssid);

        st7789_fill_screen_fb(ST7789_COLOR_BLACK);
        st7789_draw_text_fb(10, 80, "Ataque Ativo:", ST7789_COLOR_RED, ST7789_COLOR_BLACK);
        st7789_draw_text_fb(10, 110, (const char*)target->ssid, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
        st7789_draw_text_fb(10, 150, "Pressione BACK para parar", ST7789_COLOR_GRAY, ST7789_COLOR_BLACK);
        st7789_flush();

        while (gpio_get_level(BTN_BACK)) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        while(!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(20));
        evil_twin_stop_attack();

        // <-- NOVO: Ao parar o ataque, o loop recomeça e mostra a lista de novo.
    }
}
// =================================================================
// Lógica de Desenho e Controle do Menu
// =================================================================

// ✨ RESTAURADO: Funções de desenho que foram omitidas

// Função principal que desenha a tela inteira


static void draw_header_fb(const char *title) {
    st7789_fill_rect_fb(0, 0, 240, 30, ST7789_COLOR_PURPLE);
    st7789_set_text_size(2);
    st7789_draw_text_fb(10, 7, title, ST7789_COLOR_WHITE, ST7789_COLOR_PURPLE);
}



static void draw_menu_item_fb(const SubMenuItem *item, int y, bool selected) {
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




void show_wifi_submenu(void) {


    // Array de itens para o submenu de IR
    const SubMenuItem wifi_items[] = {
        // { "Label",       Icone,  Ação Final          },
        { "Scan Redes",         scan, wifi_action_scan   },
        { "Atacar Alvo",           deauth, wifi_action_attack },
        { "Evil-Twin", portal, wifi_action_evil_twin},
   
    };
    int item_count = sizeof(wifi_items) / sizeof(wifi_items[0]);

    // Chama a função da biblioteca para exibir este submenu
    show_submenu(wifi_items, item_count, " Wifi");
}
