#include "wifi.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
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
#include "wifi_analyzer.h" 
#include "traffic_analyzer.h"
#include "virtual_display_client.h" // ADICIONAR ESTE INCLUDE

// static const char *TAG = "wifi";

// --- Protótipos das Ações ---
static void wifi_action_scan(void);
static void wifi_action_attack(void);
static void wifi_action_evil_twin(void);

static void wifi_action_analyze(void); 
// --- Itens do Menu WiFi ---
static const SubMenuItem wifiMenuItems[] = {
    { "Scan Redes", scan, wifi_action_scan },
    { "Analisar Redes", analyzer_main, wifi_action_analyze }, 
    { "Analisar Trafego", analyzer_main, show_traffic_analyzer },
    { "Atacar Alvo",   deauth, wifi_action_attack },
    { "Evil Twin",     evil, wifi_action_evil_twin },
};
static const int wifiMenuSize = sizeof(wifiMenuItems) / sizeof(SubMenuItem);
static bool g_is_scanning = false; // Sinalizador para controlar a tarefa de animação
static TaskHandle_t animation_task_handle = NULL; // Handle para a tarefa
// --- Função Principal do Módulo ---
void show_wifi_menu(void) {
    wifi_init(); // REMOVER ESTA LINHA - JÁ FOI INICIALIZADA NO KERNEL
    wifi_service_init(); // Já é chamado, mas não inicializa o Wi-Fi novamente, apenas o serviço.
    show_submenu(wifiMenuItems, wifiMenuSize, "Menu WiFi");
}

// ✨ ALTERADO: Tarefa de animação que limpa a tela a cada frame
void scan_animation_task(void *pvParameters) {
    bool show_first_image = true;
    
    while (g_is_scanning) {
        // 1. Limpa completamente a tela para apagar a imagem anterior
        st7789_fill_screen_fb(ST7789_COLOR_BLACK);

        // 2. Redesenha o texto estático "Escaneando..." para que ele permaneça visível
        st7789_draw_text_fb(20, 40, "Escaneando redes...", DESIGN_PURPLE_COLOR, ST7789_COLOR_BLACK);

        // 3. Desenha a imagem correspondente a este frame da animação
        if (show_first_image) {
            st7789_draw_bitmap_fb(-17, 79, octo_ant, 200, 179, ST7789_COLOR_PURPLE);
        } else {
            st7789_draw_bitmap_fb(-17, 79, octo_ant1, 200, 179, ST7789_COLOR_PURPLE);
        }
        
        // 4. Envia o frame completo (fundo preto + texto + imagem) para o ecrã
        st7789_flush(); 
        virtual_display_notify_frame_ready(); // NOVO: Notifica a tarefa de envio de frame

        // 5. Prepara para o próximo frame
        show_first_image = !show_first_image; // Alterna para a próxima imagem
        vTaskDelay(pdMS_TO_TICKS(150)); // Controle a velocidade da animação
    }
    // Garante que a tela seja limpa quando a animação parar
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    st7789_flush();
    animation_task_handle = NULL; // Sinaliza que a tarefa terminou
    vTaskDelete(NULL);
}


// --- ✨ ALTERADO: Função de scan agora gerencia a animação ---
static void wifi_action_scan(void) {
    g_is_scanning = true; // Ativa o sinalizador para a animação começar

    // Cria e inicia a tarefa de animação
    xTaskCreate(scan_animation_task, "scan_anim_task", 4096, NULL, 5, &animation_task_handle);

    // --- Esta função é bloqueante. A animação corre em paralelo ---
    wifi_service_scan(); 
    // ----------------------------------------------------------------

    g_is_scanning = false; // Desativa o sinalizador para parar a animação
    
    // Pequeno delay para garantir que a tarefa de animação termine
    vTaskDelay(pdMS_TO_TICKS(100)); 

    // Opcional: Mostrar uma mensagem de conclusão
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    st7789_set_text_size(2);
    st7789_draw_text_fb(30, 110, "Scan Concluido!", ST7789_COLOR_GREEN, ST7789_COLOR_BLACK);
    st7789_flush();
    vTaskDelay(pdMS_TO_TICKS(1500));
}

static void wifi_action_analyze(void) {
    // Espera o botão OK ser liberado para não processar duas vezes
    while (!gpio_get_level(BTN_OK)) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    // Chama a função principal da nossa nova interface
    show_wifi_analyzer();
}

// Ação para "Atacar Alvo" com menu de seleção manual
static void wifi_action_attack(void) {
    // Espera o botão OK ser liberado para não entrar na seleção imediatamente
    while (!gpio_get_level(BTN_OK)) {
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    const int ap_count = wifi_service_get_ap_count();
    if (ap_count == 0) {
        st7789_fill_screen_fb(ST7789_COLOR_BLACK);
        st7789_set_text_size(2);
        st7789_draw_text_fb(15, 110, "Nenhuma rede escaneada!", ST7789_COLOR_RED, ST7789_COLOR_BLACK);
        st7789_flush();
        vTaskDelay(pdMS_TO_TICKS(2000));
        return;
    }

    SubMenuItem ap_menu[ap_count];
    char ap_labels[ap_count][33];

    for (int i = 0; i < ap_count; i++) {
        const wifi_ap_record_t *ap = wifi_service_get_ap_record(i);
        if (ap) {
            strncpy(ap_labels[i], (const char *)ap->ssid, sizeof(ap_labels[i]) - 1);
            ap_labels[i][sizeof(ap_labels[i]) - 1] = '\0';
            ap_menu[i].label = ap_labels[i];
            ap_menu[i].icon = wifi_main;
            ap_menu[i].action = NULL; // Ação não é necessária para um seletor
        }
    }

    int ap_selection = 0;
    int ap_offset = 0;
    bool stay_in_ap_menu = true;

    while (stay_in_ap_menu) {
        // 1. Renderiza o menu de seleção de APs
        st7789_fill_screen_fb(ST7789_COLOR_BLACK);
        menu_draw_header("Selecionar Alvo"); // Usa a função pública

        if (ap_selection < ap_offset)
            ap_offset = ap_selection;
        else if (ap_selection >= ap_offset + MAX_VISIBLE_ITEMS)
            ap_offset = ap_selection - MAX_VISIBLE_ITEMS + 1;

        for (int i = 0; i < MAX_VISIBLE_ITEMS; i++) {
            int menuIndex = i + ap_offset;
            if (menuIndex < ap_count) {
                int posY = START_Y + i * (ITEM_HEIGHT + ITEM_SPACING);
                menu_draw_item(&ap_menu[menuIndex], posY, menuIndex == ap_selection); // Usa a função pública
            }
        }
        menu_draw_scrollbar(ap_offset, ap_count, ap_selection); // Usa a função pública
        st7789_flush();

        // 2. Lida com a entrada do usuário
        bool input_processed = false;
        while (!input_processed) {
            if (!gpio_get_level(BTN_UP)) {
                while (!gpio_get_level(BTN_UP)) vTaskDelay(pdMS_TO_TICKS(200));
                ap_selection = (ap_selection - 1 + ap_count) % ap_count;
                input_processed = true;
            } else if (!gpio_get_level(BTN_DOWN)) {
                while (!gpio_get_level(BTN_DOWN)) vTaskDelay(pdMS_TO_TICKS(200));
                ap_selection = (ap_selection + 1) % ap_count;
                input_processed = true;
            } else if (!gpio_get_level(BTN_OK)) {
                while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(200));
                const wifi_ap_record_t *target_ap = wifi_service_get_ap_record(ap_selection);
                if (target_ap) {
                    // UI: Mostra mensagem de ataque
                    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
                    char attack_msg[64];
                    snprintf(attack_msg, sizeof(attack_msg), "Atacando %s...", target_ap->ssid);
                    st7789_set_text_size(2);
                    st7789_draw_text_fb(20, 110, attack_msg, ST7789_COLOR_RED, ST7789_COLOR_BLACK);
                    st7789_set_text_size(1);
                    st7789_draw_text_fb(20, 220, "Pressione BACK para parar", ST7789_COLOR_GRAY, ST7789_COLOR_BLACK);
                    st7789_flush();

                    // Loop de ataque
                    while (gpio_get_level(BTN_BACK)) {
                        wifi_deauther_send_deauth_frame(target_ap, 1); // Envia frame
                        vTaskDelay(pdMS_TO_TICKS(5));
                    }
                    while(!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(200)); // Aguarda liberação do botão
                }
                stay_in_ap_menu = false;
                input_processed = true;
            } else if (!gpio_get_level(BTN_BACK)) {
                while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(200));
                stay_in_ap_menu = false;
                input_processed = true;
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}



// Ação para "Evil Twin" com menu de seleção manual
static void wifi_action_evil_twin(void) {
    while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(20));

    const int ap_count = wifi_service_get_ap_count();
    if (ap_count == 0) {
        st7789_fill_screen_fb(ST7789_COLOR_BLACK);
        st7789_set_text_size(2);
        st7789_draw_text_fb(15, 110, "Nenhuma rede escaneada!", ST7789_COLOR_RED, ST7789_COLOR_BLACK);
        st7789_flush();
        vTaskDelay(pdMS_TO_TICKS(2000));
        return;
    }

    SubMenuItem ap_menu[ap_count];
    char ap_labels[ap_count][33];

    for (int i = 0; i < ap_count; i++) {
        const wifi_ap_record_t *ap = wifi_service_get_ap_record(i);
        if (ap) {
            strncpy(ap_labels[i], (const char *)ap->ssid, sizeof(ap_labels[i]) - 1);
            ap_labels[i][sizeof(ap_labels[i]) - 1] = '\0';
            ap_menu[i].label = ap_labels[i];
            ap_menu[i].icon = wifi_main;
            ap_menu[i].action = NULL;
        }
    }

    int ap_selection = 0;
    int ap_offset = 0;
    bool stay_in_menu = true;

    while (stay_in_menu) {
        st7789_fill_screen_fb(ST7789_COLOR_BLACK);
        menu_draw_header("Alvo para Evil Twin"); // Usa a função pública

        if (ap_selection < ap_offset) ap_offset = ap_selection;
        else if (ap_selection >= ap_offset + MAX_VISIBLE_ITEMS) ap_offset = ap_selection - MAX_VISIBLE_ITEMS + 1;

        for (int i = 0; i < MAX_VISIBLE_ITEMS; i++) {
            int menuIndex = i + ap_offset;
            if (menuIndex < ap_count) {
                int posY = START_Y + i * (ITEM_HEIGHT + ITEM_SPACING);
                menu_draw_item(&ap_menu[menuIndex], posY, menuIndex == ap_selection); // Usa a função pública
            }
        }
        menu_draw_scrollbar(ap_offset, ap_count, ap_selection); // Usa a função pública
        st7789_flush();

        bool input_processed = false;
        while (!input_processed) {
            if (!gpio_get_level(BTN_UP)) {
                while (!gpio_get_level(BTN_UP)) vTaskDelay(pdMS_TO_TICKS(50));
                ap_selection = (ap_selection - 1 + ap_count) % ap_count;
                input_processed = true;
            } else if (!gpio_get_level(BTN_DOWN)) {
                while (!gpio_get_level(BTN_DOWN)) vTaskDelay(pdMS_TO_TICKS(50));
                ap_selection = (ap_selection + 1) % ap_count;
                input_processed = true;
            } else if (!gpio_get_level(BTN_OK)) {
                while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(50));
                const wifi_ap_record_t *target = wifi_service_get_ap_record(ap_selection);
                if (target) {
                    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
                    st7789_draw_text_fb(10, 80, "Iniciando Evil Twin...", ST7789_COLOR_RED, ST7789_COLOR_BLACK);
                    st7789_flush();

                    evil_twin_start_attack((const char *)target->ssid);

                    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
                    st7789_set_text_size(2);
                    st7789_draw_text_fb(10, 80, "Ataque Ativo:", ST7789_COLOR_RED, ST7789_COLOR_BLACK);
                    st7789_draw_text_fb(10, 110, (const char *)target->ssid, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
                     st7789_set_text_size(1);
                    st7789_draw_text_fb(10, 150, "Pressione BACK para parar", ST7789_COLOR_GRAY, ST7789_COLOR_BLACK);
                    st7789_flush();

                    while (gpio_get_level(BTN_BACK)) {
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                    while(!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(20));
                    evil_twin_stop_attack();
                }
                input_processed = true;
                stay_in_menu = false;
            } else if (!gpio_get_level(BTN_BACK)) {
                while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(50));
                stay_in_menu = false;
                input_processed = true;
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}
