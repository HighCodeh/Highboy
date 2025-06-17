
#include "sub_menu.h"
#include "st7789.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "icons.h"

#define BTN_UP     15
#define BTN_DOWN   6
#define BTN_OK     4
#define BTN_BACK   7

#define MAX_VISIBLE_ITEMS 4
#define ITEM_HEIGHT 55
#define ITEM_WIDTH 220
#define START_Y 40
#define ITEM_SPACING 8

static int currentSelection = 0;
static int offset = 0;
static bool fundoDesenhado = false;

static const MenuItem *currentMenuItems = NULL;
static int currentMenuSize = 0;
static const char *currentMenuTitle = NULL;

static void draw_header(const char *title);
static void draw_menu_items(const MenuItem *items, int itemCount);
static void draw_menu_item(const MenuItem *item, int y, bool selected);
static void draw_scroll_bar(int itemCount);

void show_menu(const MenuItem *items, int itemCount, const char *title) {
    fundoDesenhado = false;
    currentSelection = 0;
    offset = 0;

    currentMenuItems = items;
    currentMenuSize = itemCount;
    currentMenuTitle = title;

    st7789_fill_screen(ST7789_COLOR_BLACK);
    draw_header(title);
    draw_menu_items(items, itemCount);
    draw_scroll_bar(itemCount);
    fundoDesenhado = true;
}

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
            fundoDesenhado = false;
            currentMenuItems[currentSelection].action();
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        else if (!gpio_get_level(BTN_BACK)) {
            fundoDesenhado = false;
            stayInMenu = false;
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        if (updated) {
            st7789_fill_screen(ST7789_COLOR_BLACK);
            draw_header(currentMenuTitle);
            draw_menu_items(currentMenuItems, currentMenuSize);
            draw_scroll_bar(currentMenuSize);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void draw_header(const char *title) {
    st7789_draw_rect(0, 0, 240, 30, ST7789_COLOR_PURPLE);
    st7789_set_text_size(2);
    st7789_draw_text(40, 7, title, ST7789_COLOR_WHITE);
}

static void draw_menu_items(const MenuItem *items, int itemCount) {
    if (currentSelection < offset) {
        offset = currentSelection;
    } else if (currentSelection >= offset + MAX_VISIBLE_ITEMS) {
        offset = currentSelection - MAX_VISIBLE_ITEMS + 1;
    }

    for (int i = 0; i < MAX_VISIBLE_ITEMS; i++) {
        int menuIndex = i + offset;
        if (menuIndex < itemCount) {
            int posY = START_Y + i * (ITEM_HEIGHT + ITEM_SPACING);
            draw_menu_item(&items[menuIndex], posY, menuIndex == currentSelection);
        }
    }
}

static void draw_menu_item(const MenuItem *item, int posY, bool selected) {
    int iconSize = 24;
    int textX = selected ? 65 : 50;
    int iconX = selected ? 35 : 20;
    int iconY = posY + (ITEM_HEIGHT - iconSize) / 2;

    st7789_draw_rect(10, posY, ITEM_WIDTH, ITEM_HEIGHT, ST7789_COLOR_BLACK);
    st7789_draw_round_rect(10, posY, ITEM_WIDTH, ITEM_HEIGHT, 10, selected ? ST7789_COLOR_LIGHT_PURPLE : ST7789_COLOR_PURPLE);
    st7789_draw_text(textX, posY + 15, item->label, selected ? ST7789_COLOR_LIGHT_PURPLE : ST7789_COLOR_WHITE);

    if (item->icon != NULL) {
        st7789_draw_bitmapRGB(iconX, iconY, item->icon, iconSize, iconSize);
    }
}

static void draw_scroll_bar(int itemCount) {
    int usableHeight = ST7789_HEIGHT - START_Y;
    int scrollBarWidth = 5;
    int spacing = 5;

    st7789_draw_rect(ST7789_WIDTH - scrollBarWidth, START_Y, scrollBarWidth, usableHeight, ST7789_COLOR_BLACK);

    for (int y = START_Y; y < ST7789_HEIGHT; y += spacing) {
        st7789_draw_pixel(ST7789_WIDTH - scrollBarWidth / 2, y, ST7789_COLOR_DARKGRAY);
    }

    float scrollBarHeight = (float)usableHeight * ((float)MAX_VISIBLE_ITEMS / itemCount);
    if (scrollBarHeight < 8) scrollBarHeight = 8;

    float maxScrollArea = usableHeight - scrollBarHeight;
    float scrollY = ((float)offset / (itemCount - MAX_VISIBLE_ITEMS)) * maxScrollArea;
    if (scrollY > maxScrollArea) scrollY = maxScrollArea;

    st7789_fill_round_rect(
        ST7789_WIDTH - scrollBarWidth,
        START_Y + (int)scrollY,
        scrollBarWidth,
        (int)scrollBarHeight,
        3,
        ST7789_COLOR_WHITE
    );
}

void wifi_scan(void) {
    printf("Executando scan WiFi...\n");
}

void show_wifi_menu() {
    MenuItem wifiItems[] = {
        { "Scan", wifi_main, wifi_scan },
        { "Ataque", wifi_main, NULL },
        { "Scan", wifi_main, wifi_scan },
        { "Ataque", wifi_main, NULL },
        { "Scan", wifi_main, wifi_scan },
        { "Ataque", wifi_main, NULL },
        { "Scan", wifi_main, wifi_scan },
        { "Ataque", wifi_main, NULL }
    };
    show_menu(wifiItems, 8, "WiFi");
    handle_menu_controls();
}
