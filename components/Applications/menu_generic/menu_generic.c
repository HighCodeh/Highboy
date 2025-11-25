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

#include "menu_generic.h"
#include "st7789.h"
#include "pin_def.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "menu.h"
#include <string.h>

#define ITEM_HEIGHT         50
#define ITEM_WIDTH          220
#define ITEM_SPACING        8
#define SCROLL_BAR_WIDTH    5


static void draw_menu_item(Menu* menu, int index, int posY, bool isSelected) {
    const MenuItem* item = &menu->items[index];

    uint16_t rect_color = isSelected ? ST7789_COLOR_WHITE : ST7789_COLOR_PURPLE;
    
    const int iconSize = 24;
    const int fontHeight = 16;
    const int padding = 10;

    int contentHeight = iconSize > fontHeight ? iconSize : fontHeight;
    int contentY = posY + (ITEM_HEIGHT - contentHeight) / 2;
    int iconY = contentY + (contentHeight - iconSize) / 2;
    int textY = contentY + (contentHeight - fontHeight) / 2;
    int itemX = (240 - ITEM_WIDTH) / 2;

    
    st7789_draw_round_rect_fb(itemX, posY, ITEM_WIDTH, ITEM_HEIGHT, 8, rect_color);

    
    int iconX = itemX + 20;
    int textX = iconX + iconSize + 10;

    if (item->icon != NULL) {
        st7789_draw_image_fb(iconX, iconY, iconSize, iconSize, item->icon);
    }
    st7789_draw_text_fb(textX, textY, item->label, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);

    
    if (isSelected) {
        int circleX = itemX + ITEM_WIDTH - 20;  
        int circleY = posY + ITEM_HEIGHT / 2;
        st7789_fill_circle_fb(circleX, circleY, 4, ST7789_COLOR_WHITE);  
    }
}



static void draw_scroll_bar(Menu* menu, int scrollOffset) {
    const int maxVisibleItems = 4;

    if (menu->item_count <= maxVisibleItems) {
        return; 
    }

    int screenHeight = ST7789_HEIGHT;
    int usableHeight = screenHeight;

    int spacing = 5;
    for (int y = 0; y < screenHeight; y += spacing) {
        st7789_draw_pixel_fb(ST7789_WIDTH - SCROLL_BAR_WIDTH / 2, y, ST7789_COLOR_GRAY);
    }

    float scrollBarHeight = (float)usableHeight * ((float)maxVisibleItems / menu->item_count);
    if (scrollBarHeight < 6) scrollBarHeight = 6;

    float maxScrollArea = usableHeight - scrollBarHeight;
    float scrollY = ((float)scrollOffset / (menu->item_count - maxVisibleItems)) * maxScrollArea;
    if (scrollY > maxScrollArea) scrollY = maxScrollArea;

    st7789_fill_round_rect_fb(
        237,                  
        (int)scrollY,         
        6,                    
        (int)scrollBarHeight, 
        3,                    
        ST7789_COLOR_PURPLE   
    );
}
    


void show_menu(Menu* menu) {
    const int maxVisibleItems = 4;
    const int startY = 10;
    const int itemHeight = 60;

    int selectedIndex = 0;
    int scrollOffset = 0;

    bool inMenu = true;

    while (inMenu) {
        st7789_fill_screen_fb(ST7789_COLOR_BLACK);

        if (selectedIndex < scrollOffset) {
            scrollOffset = selectedIndex;
        } else if (selectedIndex >= scrollOffset + maxVisibleItems) {
            scrollOffset = selectedIndex - maxVisibleItems + 1;
        }

        for (int i = 0; i < maxVisibleItems; i++) {
            int menuIndex = i + scrollOffset;
            if (menuIndex < menu->item_count) {
                draw_menu_item(menu, menuIndex, startY + i * itemHeight, menuIndex == selectedIndex);
            }
        }

        draw_scroll_bar(menu, scrollOffset);

        st7789_flush();

        if (!gpio_get_level(BTN_UP)) {
            selectedIndex = (selectedIndex - 1 + menu->item_count) % menu->item_count;
            vTaskDelay(pdMS_TO_TICKS(150));
        } else if (!gpio_get_level(BTN_DOWN)) {
            selectedIndex = (selectedIndex + 1) % menu->item_count;
            vTaskDelay(pdMS_TO_TICKS(150));
        } else if (!gpio_get_level(BTN_OK)) {
            if (menu->items[selectedIndex].action)
                menu->items[selectedIndex].action();
            vTaskDelay(pdMS_TO_TICKS(200));
        } else if (!gpio_get_level(BTN_BACK)) {
            inMenu = false;
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
