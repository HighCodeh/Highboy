#include "sub_menu.h"
#include "st7789.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "icons.h"
#include "pin_def.h"
// Definições dos botões e layout
#define MAX_VISIBLE_ITEMS   4
#define ITEM_HEIGHT         55
#define ITEM_WIDTH          220
#define START_Y             40
#define ITEM_SPACING        8
#define SCROLL_BAR_WIDTH    5
#define SCROLL_BAR_TOP      START_Y

// Variáveis de estado do menu
static int currentSelection = 0;
static int offset = 0;
static const MenuItem *currentMenuItems = NULL;
static int currentMenuSize = 0;
static const char *currentMenuTitle = NULL;

// Declarações de funções estáticas
static void render_current_menu(void);
static void draw_header_fb(const char *title);
static void draw_menu_items_fb(void);
static void draw_menu_item_fb(const MenuItem *item, int y, bool selected);
static void draw_scroll_bar_fb(void);
static void executeMenuItem(int index); // Adicionada declaração

// Função principal que desenha a tela inteira e a envia para o display
static void render_current_menu(void) {
    // 1. Limpa o framebuffer
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);

    // 2. Desenha o cabeçalho
    draw_header_fb(currentMenuTitle);

    // 3. Desenha os itens do menu
    draw_menu_items_fb();
    
    // 4. Desenha a barra de rolagem
    draw_scroll_bar_fb();

    // 5. Envia o buffer completo para a tela
    st7789_flush();
}

// Prepara e exibe um novo menu
void show_menu(const MenuItem *items, int itemCount, const char *title) {
    // Reseta o estado para o novo menu
    currentSelection = 0;
    offset = 0;
    currentMenuItems = items;
    currentMenuSize = itemCount;
    currentMenuTitle = title;

    // Desenha o estado inicial do menu
    render_current_menu();
}

// Loop de controle do menu
void handle_menu_controls(void) {
    bool stayInMenu = true;
    while (stayInMenu) {
        bool updated = false;

        if (!gpio_get_level(BTN_UP)) {
            currentSelection = (currentSelection - 1 + currentMenuSize) % currentMenuSize;
            updated = true;
            vTaskDelay(pdMS_TO_TICKS(150));
        }
        else if (!gpio_get_level(BTN_DOWN)) {
            currentSelection = (currentSelection + 1) % currentMenuSize;
            updated = true;
            vTaskDelay(pdMS_TO_TICKS(150));
        }
        else if (!gpio_get_level(BTN_OK)) {
            // Executa a ação e redesenha o menu ao retornar
            executeMenuItem(currentSelection);
            render_current_menu();
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        else if (!gpio_get_level(BTN_BACK)) {
            stayInMenu = false;
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        // Se a seleção mudou, redesenha a tela
        if (updated) {
            render_current_menu();
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Desenha o cabeçalho no framebuffer
static void draw_header_fb(const char *title) {
    st7789_fill_rect_fb(0, 0, 240, 30, ST7789_COLOR_PURPLE);
    st7789_set_text_size(2);
    st7789_draw_text_fb(40, 7, title, ST7789_COLOR_WHITE, ST7789_COLOR_PURPLE);
}

// Desenha os itens visíveis no framebuffer
static void draw_menu_items_fb(void) {
    // Ajusta o offset para manter o item selecionado visível
    if (currentSelection < offset) {
        offset = currentSelection;
    } else if (currentSelection >= offset + MAX_VISIBLE_ITEMS) {
        offset = currentSelection - MAX_VISIBLE_ITEMS + 1;
    }

    // Desenha cada item visível
    for (int i = 0; i < MAX_VISIBLE_ITEMS; i++) {
        int menuIndex = i + offset;
        if (menuIndex < currentMenuSize) {
            int posY = START_Y + i * (ITEM_HEIGHT + ITEM_SPACING);
            draw_menu_item_fb(&currentMenuItems[menuIndex], posY, menuIndex == currentSelection);
        }
    }
}

// Desenha um único item no framebuffer
static void draw_menu_item_fb(const MenuItem *item, int posY, bool selected) {
    int iconSize = 24;
    int textX = selected ? 65 : 50;
    int iconX = selected ? 35 : 20;
    int iconY = posY + (ITEM_HEIGHT - iconSize) / 2;
    uint16_t rect_color = selected ? ST7789_COLOR_LIGHT_PURPLE : ST7789_COLOR_PURPLE;
    uint16_t text_color = selected ? ST7789_COLOR_LIGHT_PURPLE : ST7789_COLOR_WHITE;

    st7789_draw_round_rect_fb(10, posY, ITEM_WIDTH, ITEM_HEIGHT, 10, rect_color);
    st7789_draw_text_fb(textX, posY + 15, item->label, text_color, ST7789_COLOR_BLACK);

    if (item->icon != NULL) {
        st7789_draw_image_fb(iconX, iconY, iconSize, iconSize, item->icon);
    }
}

// Desenha a barra de rolagem no framebuffer
static void draw_scroll_bar_fb(void) {
    if (currentMenuSize <= MAX_VISIBLE_ITEMS) return; // Não desenha se não há rolagem

    int usableHeight = ST7789_HEIGHT - START_Y;
    int spacing = 5;

    for (int y = START_Y; y < ST7789_HEIGHT; y += spacing) {
        st7789_draw_pixel_fb(ST7789_WIDTH - SCROLL_BAR_WIDTH / 2, y, ST7789_COLOR_DARKGRAY);
    }

    float scrollBarHeight = (float)usableHeight * ((float)MAX_VISIBLE_ITEMS / currentMenuSize);
    if (scrollBarHeight < 8) scrollBarHeight = 8;

    float maxScrollArea = usableHeight - scrollBarHeight;
    float scrollY = ((float)offset / (currentMenuSize - MAX_VISIBLE_ITEMS)) * maxScrollArea;
    if (scrollY > maxScrollArea) scrollY = maxScrollArea;

    st7789_fill_round_rect_fb(
        ST7789_WIDTH - SCROLL_BAR_WIDTH,
        START_Y + (int)scrollY,
        SCROLL_BAR_WIDTH,
        (int)scrollBarHeight,
        3,
        ST7789_COLOR_WHITE
    );
}

// Função de exemplo para o menu WiFi
// void wifi_scan(void) {
//     // Limpa a tela para mostrar a mensagem de scan
//     st7789_fill_screen_fb(ST7789_COLOR_BLACK);
//     st7789_draw_text_fb(20, 100, "Escaneando WiFi...", ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
//     st7789_flush();
//     vTaskDelay(pdMS_TO_TICKS(2000)); // Simula o scan
// }

// Monta e exibe o menu WiFi
// void show_wifi_menu() {
//     MenuItem wifiItems[] = {
//         { "Scan", wifi_main, wifi_scan },
//         { "Ataque", wifi_main, NULL },
//         { "Info", wifi_main, NULL },
//         { "Outro", wifi_main, NULL },
//         { "Item 5", wifi_main, NULL },
//     };
//     int itemCount = sizeof(wifiItems) / sizeof(wifiItems[0]);
//     show_menu(wifiItems, itemCount, "WiFi");
//     handle_menu_controls();
// }

// Executa a ação do item de menu selecionado
static void executeMenuItem(int index) {
    if (currentMenuItems && index < currentMenuSize && currentMenuItems[index].action) {
        currentMenuItems[index].action();
    }
}

