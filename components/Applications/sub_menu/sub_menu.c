// Arquivo: sub_menu.c (Versão Unificada)
#include "sub_menu.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pin_def.h"
#include <string.h>

// --- Constantes de Layout ---
#define MAX_VISIBLE_ITEMS 4
#define ITEM_HEIGHT 45
#define ITEM_WIDTH 220
#define START_Y 40
#define ITEM_SPACING 5
#define SCROLL_BAR_WIDTH 5
#define HEADER_HEIGHT 30
#define ICON_SIZE 24


#define COLOR_BACKGROUND ST7789_COLOR_BLACK
#define COLOR_PURPLE 0x991D
#define COLOR_TEXT ST7789_COLOR_WHITE
#define COLOR_SCROLL_TRACK ST7789_COLOR_DARKGRAY
#define COLOR_SCROLL_THUMB COLOR_PURPLE


static int currentSelection = 0;
static int offset = 0;
static const SubMenuItem *currentMenuItems = NULL;
static int currentMenuSize = 0;
static const char *currentMenuTitle = NULL;


static void draw_header(void);
static void draw_menu_items(void);
static void draw_menu_item(int menuIndex, int y, bool selected);
static void draw_scroll_bar(void);
static void render_full_menu(void);

// --- A NOVA FUNÇÃO DE CONTROLE ÚNICA ---
static int handle_menu_controls(void) {

    static bool botoes_ja_configurados = false;
    if (!botoes_ja_configurados) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << BTN_UP) | (1ULL << BTN_DOWN) | (1ULL << BTN_OK) | (1ULL << BTN_BACK),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
        botoes_ja_configurados = true;
    }
    
    // Aguarda soltar qualquer botão pressionado ao entrar
    while (!gpio_get_level(BTN_OK) || !gpio_get_level(BTN_DOWN) || !gpio_get_level(BTN_UP) || !gpio_get_level(BTN_BACK)) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // Loop principal de controle de input
    while (1) {
        bool needs_redraw = false;

        if (!gpio_get_level(BTN_DOWN)) {
            while (!gpio_get_level(BTN_DOWN)) vTaskDelay(pdMS_TO_TICKS(20));
            currentSelection = (currentSelection + 1) % currentMenuSize;
            needs_redraw = true;
        } else if (!gpio_get_level(BTN_UP)) {
            while (!gpio_get_level(BTN_UP)) vTaskDelay(pdMS_TO_TICKS(20));
            currentSelection = (currentSelection > 0) ? currentSelection - 1 : currentMenuSize - 1;
            needs_redraw = true;
        } else if (!gpio_get_level(BTN_OK)) {
            while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(20));
            return currentSelection; // Retorna o índice selecionado
        } else if (!gpio_get_level(BTN_BACK)) {
            while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(20));
            return -1; // Retorna -1 para "Voltar"
        }

        if (needs_redraw) {
            draw_menu_items();
            draw_scroll_bar();
            st7789_flush();
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


void show_submenu(const SubMenuItem *items, int itemCount, const char *title) {

    while (1) {
        currentMenuItems = items;
        currentMenuSize = itemCount;
        currentMenuTitle = title;
        currentSelection = 0; 
        offset = 0;

        render_full_menu();
        
        int selected_index = handle_menu_controls();

        if (selected_index == -1) {
            break; 
        }

        if (items[selected_index].action != NULL) {
            items[selected_index].action();
        }
        
    }

    currentMenuItems = NULL;
    currentMenuSize = 0;
    currentMenuTitle = NULL;
}


int show_selection_menu(const SubMenuItem *items, int itemCount, const char *title) {
    currentMenuItems = items;
    currentMenuSize = itemCount;
    currentMenuTitle = title;
    currentSelection = 0;
    offset = 0;

    render_full_menu();
    int result = handle_menu_controls();

    currentMenuItems = NULL;
    currentMenuSize = 0;
    currentMenuTitle = NULL;

    return result;
}


static void render_full_menu(void) {
    st7789_fill_screen_fb(COLOR_BACKGROUND);
    draw_header();
    draw_scroll_bar();
    draw_menu_items();
    st7789_flush();
}

static void draw_header(void) {
    st7789_fill_rect_fb(0, 0, ST7789_WIDTH, HEADER_HEIGHT, COLOR_PURPLE);
    if (currentMenuTitle != NULL) {
        st7789_set_text_size(2);
        int text_width = strlen(currentMenuTitle) * 12;
        int text_x = (ST7789_WIDTH - text_width) / 2;
        int text_y = (HEADER_HEIGHT - 16) / 2;
        if (text_x < 0) text_x = 2;
        st7789_draw_text_fb(text_x, text_y, currentMenuTitle, COLOR_TEXT, COLOR_PURPLE);
    }
}

static void draw_menu_items(void) {
    if (currentMenuSize == 0) return; // Proteção contra divisão por zero
    if (currentSelection < offset) {
        offset = currentSelection;
    } else if (currentSelection >= offset + MAX_VISIBLE_ITEMS) {
        offset = currentSelection - MAX_VISIBLE_ITEMS + 1;
    }
    st7789_fill_rect_fb(0, START_Y, ST7789_WIDTH, ST7789_HEIGHT - START_Y, COLOR_BACKGROUND);
    for (int i = 0; i < MAX_VISIBLE_ITEMS; i++) {
        int menuIndex = i + offset;
        if (menuIndex < currentMenuSize) {
            int posY = START_Y + i * (ITEM_HEIGHT + ITEM_SPACING);
            draw_menu_item(menuIndex, posY, menuIndex == currentSelection);
        }
    }
}

static void draw_menu_item(int menuIndex, int y, bool selected) {
    const SubMenuItem *item = &currentMenuItems[menuIndex];
    int item_x = 10;
    int icon_x = item_x + 10;
    int text_x = icon_x + ICON_SIZE + 10;
    int text_y = y + (ITEM_HEIGHT - 16) / 2;
    int icon_y = y + (ITEM_HEIGHT - ICON_SIZE) / 2;
    if (selected) {
        st7789_draw_round_rect_fb(item_x, y, ITEM_WIDTH, ITEM_HEIGHT, 10, COLOR_TEXT);
        int circle_x = item_x + ITEM_WIDTH - 20;
        int circle_y = y + ITEM_HEIGHT / 2;
        st7789_fill_circle_fb(circle_x, circle_y, 4, COLOR_TEXT);
    } else {
        st7789_draw_round_rect_fb(item_x, y, ITEM_WIDTH, ITEM_HEIGHT, 10, COLOR_PURPLE);
    }
    if (item->icon) {
        st7789_draw_image_fb(icon_x, icon_y, ICON_SIZE, ICON_SIZE, item->icon);
    }
    st7789_set_text_size(2);
    st7789_draw_text_fb(text_x, text_y, item->label, COLOR_TEXT, COLOR_BACKGROUND);
}

static void draw_scroll_bar(void) {
    st7789_fill_rect_fb(ST7789_WIDTH - SCROLL_BAR_WIDTH - 2, 0, SCROLL_BAR_WIDTH + 2, ST7789_HEIGHT, COLOR_BACKGROUND);
    if (currentMenuSize <= MAX_VISIBLE_ITEMS) return;
    for (int y = 0; y < ST7789_HEIGHT; y += 5) {
        st7789_draw_pixel_fb(ST7789_WIDTH - SCROLL_BAR_WIDTH / 2 - 1, y, COLOR_SCROLL_TRACK);
    }
    int scroll_area_height = ST7789_HEIGHT - START_Y;
    float thumb_height = (float)scroll_area_height * ((float)MAX_VISIBLE_ITEMS / currentMenuSize);
    if (thumb_height < 10) thumb_height = 10;
    float scroll_track_space = scroll_area_height - thumb_height;
    float scroll_y_pos = (currentMenuSize > MAX_VISIBLE_ITEMS) ? ((float)offset / (currentMenuSize - MAX_VISIBLE_ITEMS)) * scroll_track_space : 0;
    st7789_fill_round_rect_fb(ST7789_WIDTH - SCROLL_BAR_WIDTH - 1, START_Y + (int)scroll_y_pos, SCROLL_BAR_WIDTH, (int)thumb_height, 2, COLOR_SCROLL_THUMB);
}