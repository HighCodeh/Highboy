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

#include "sub_menu.h" // Inclui as novas declarações públicas

// Dependências
#include "st7789.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "icons.h"
#include "pin_def.h"
#include "esp_log.h"

// Estrutura de estado interna
typedef struct {
    int currentSelection;
    int offset;
    const SubMenuItem *items;
    int itemCount;
    const char *title;
} SubMenuState;

#define MAX_SUBMENU_DEPTH 5
static SubMenuState subMenuStack[MAX_SUBMENU_DEPTH];
static int subMenuStackPtr = -1;
static const char* TAG = "submenu";

// ✨ ALTERAÇÃO: Constantes removidas daqui pois agora estão no sub_menu.h

// Protótipos de funções que ainda são privadas
static void handle_submenu_controls(void);
static void render_current_submenu(void);
static void executeSubMenuItem(int index);
static void draw_submenu_items(void);
static int handle_picker_controls(void);

// Função principal pública
void show_submenu(const SubMenuItem *items, int itemCount, const char *title) {
    if (subMenuStackPtr >= MAX_SUBMENU_DEPTH - 1) {
        ESP_LOGE(TAG, "Profundidade máxima de sub-menu atingida!");
        return;
    }
    subMenuStackPtr++;
    SubMenuState *currentState = &subMenuStack[subMenuStackPtr];
    currentState->items = items;
    currentState->itemCount = itemCount;
    currentState->title = title;
    currentState->currentSelection = 0;
    currentState->offset = 0;
    render_current_submenu();
    handle_submenu_controls();
}

// ... (handle_submenu_controls e executeSubMenuItem permanecem iguais)
static void handle_submenu_controls(void) {
    bool stayInSubMenu = true;
    while (stayInSubMenu) {
        SubMenuState *currentState = &subMenuStack[subMenuStackPtr];
        bool updated = false;
        if (!gpio_get_level(BTN_UP)) {
            while (!gpio_get_level(BTN_UP)) vTaskDelay(pdMS_TO_TICKS(50));
            currentState->currentSelection = (currentState->currentSelection - 1 + currentState->itemCount) % currentState->itemCount;
            updated = true;
        } else if (!gpio_get_level(BTN_DOWN)) {
            while (!gpio_get_level(BTN_DOWN)) vTaskDelay(pdMS_TO_TICKS(50));
            currentState->currentSelection = (currentState->currentSelection + 1) % currentState->itemCount;
            updated = true;
        } else if (!gpio_get_level(BTN_OK)) {
            while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(800));
            executeSubMenuItem(currentState->currentSelection);
            if (subMenuStackPtr >= 0) {
                render_current_submenu();
            }
        } else if (!gpio_get_level(BTN_BACK)) {
            while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(50));
            stayInSubMenu = false;
        }
        if (updated) {
            render_current_submenu();
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    subMenuStackPtr--;
}

static void executeSubMenuItem(int index) {
    SubMenuState *currentState = &subMenuStack[subMenuStackPtr];
    if (currentState->items && index < currentState->itemCount && currentState->items[index].action) {
        currentState->items[index].action();
    }
}


// Função de renderização principal
static void render_current_submenu(void) {
    if (subMenuStackPtr < 0) return;
    SubMenuState *currentState = &subMenuStack[subMenuStackPtr];
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    menu_draw_header(currentState->title); // ✨ Usa a função pública
    draw_submenu_items();
    menu_draw_scrollbar(currentState->offset, currentState->itemCount, currentState->currentSelection); // ✨ Usa a função pública
    st7789_flush();
}

// Loop de desenho dos itens (continua privado)
static void draw_submenu_items(void) {
    SubMenuState *currentState = &subMenuStack[subMenuStackPtr];
    if (currentState->currentSelection < currentState->offset) {
        currentState->offset = currentState->currentSelection;
    } else if (currentState->currentSelection >= currentState->offset + MAX_VISIBLE_ITEMS) {
        currentState->offset = currentState->currentSelection - MAX_VISIBLE_ITEMS + 1;
    }
    for (int i = 0; i < MAX_VISIBLE_ITEMS; i++) {
        int menuIndex = i + currentState->offset;
        if (menuIndex < currentState->itemCount) {
            int posY = START_Y + i * (ITEM_HEIGHT + ITEM_SPACING);
            menu_draw_item(&currentState->items[menuIndex], posY, menuIndex == currentState->currentSelection); // ✨ Usa a função pública
        }
    }
}


// --- FUNÇÕES DE DESENHO PÚBLICAS ---

// ✨ ALTERAÇÃO: A palavra 'static' foi removida e o nome alterado para corresponder ao .h
void menu_draw_header(const char *title) {
    st7789_draw_rect_fb(0, 0, 240, 30, DESIGN_PURPLE_COLOR);
    st7789_fill_rect_fb(1, 1, 238, 28, ST7789_COLOR_BLACK);
    st7789_set_text_size(2);
    if (title) {
        st7789_draw_text_fb(40, 7, title, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    }
}

// ✨ ALTERAÇÃO: A palavra 'static' foi removida e o nome alterado
// ✨ ALTERAÇÃO: Função de desenho de item com novo efeito de seleção
void menu_draw_item(const SubMenuItem *item, int posY, bool selected) {
    const int itemHeight = 45;
    const int iconSize = 24;
    const int textX = 50, iconX = 20;

    // Define as cores com base no estado de seleção
    uint16_t borderColor = selected ? ST7789_COLOR_WHITE : DESIGN_PURPLE_COLOR;
    uint16_t textColor = ST7789_COLOR_WHITE; // Texto sempre branco para maior contraste

    // Desenha o fundo e a borda do item
    st7789_fill_rect_fb(10, posY, ITEM_WIDTH, itemHeight, ST7789_COLOR_BLACK);
    st7789_draw_round_rect_fb(10, posY, ITEM_WIDTH, itemHeight, 10, borderColor);

    // Desenha o texto do item
    st7789_set_text_size(2);
    st7789_draw_text_fb(textX, posY + 15, item->label, textColor, ST7789_COLOR_BLACK);

    // Desenha o ícone, se existir
    if (item->icon) {
        int iconY = posY + (itemHeight - iconSize) / 2;
        st7789_draw_image_fb(iconX, iconY, iconSize, iconSize, item->icon);
    }
    
    // ✨ NOVO: Desenha a bolinha de seleção no canto direito se o item estiver selecionado
    if (selected) {
        int circleX = 10 + ITEM_WIDTH - 20; // Posição X da bolinha
        int circleY = posY + itemHeight / 2; // Posição Y (centro vertical)
        int circleRadius = 5;                // Raio da bolinha
        st7789_fill_circle_fb(circleX, circleY, circleRadius, ST7789_COLOR_WHITE);
    }
}

// ✨ ALTERAÇÃO: A palavra 'static' foi removida e o nome alterado
void menu_draw_scrollbar(int current_offset, int total_items, int current_selection) {
    if (total_items <= MAX_VISIBLE_ITEMS) return;

    st7789_fill_rect_fb(ST7789_WIDTH - SCROLL_BAR_WIDTH, START_Y, SCROLL_BAR_WIDTH, ST7789_HEIGHT - START_Y, ST7789_COLOR_BLACK);
    int spacing = 5;
    for (int y = START_Y; y < ST7789_HEIGHT; y += spacing) {
        st7789_draw_pixel_fb(ST7789_WIDTH - SCROLL_BAR_WIDTH / 2, y, ST7789_COLOR_WHITE);
    }
    float scrollBarHeight = (float)(ST7789_HEIGHT - START_Y) / total_items;
    if (scrollBarHeight < 2) scrollBarHeight = 2;
    float maxScrollHeight = ST7789_HEIGHT - START_Y - scrollBarHeight;
    float scrollBarY = (float)current_selection / (total_items - 1) * maxScrollHeight;
    if (scrollBarY > maxScrollHeight) scrollBarY = maxScrollHeight;
    st7789_fill_rect_fb(ST7789_WIDTH - SCROLL_BAR_WIDTH, START_Y + scrollBarY, SCROLL_BAR_WIDTH, scrollBarHeight, ST7789_COLOR_WHITE);
}

int show_picker_menu(const SubMenuItem *items, int itemCount, const char *title) {
    if (subMenuStackPtr >= MAX_SUBMENU_DEPTH - 1) {
        ESP_LOGE(TAG, "Profundidade máxima de menu atingida!");
        return -1;
    }
    
    // Empilha o estado do menu
    subMenuStackPtr++;
    SubMenuState *currentState = &subMenuStack[subMenuStackPtr];
    currentState->items = items;
    currentState->itemCount = itemCount;
    currentState->title = title;
    currentState->currentSelection = 0;
    currentState->offset = 0;
    
    // Renderiza e entra no loop de controle do seletor
    render_current_submenu(); // Reutiliza a função de renderização!
    int selection = handle_picker_controls();
    
    subMenuStackPtr--; // Desempilha
    
    // Restaura a tela do menu anterior, se houver
    if (subMenuStackPtr >= 0) {
        render_current_submenu();
    }
    
    return selection;
}

/**
 * @brief Loop de controle para o menu seletor que retorna um índice.
 */
static int handle_picker_controls(void) {
    int selection_result = -1; // Padrão é -1 (BACK)

    while (true) {
        SubMenuState *currentState = &subMenuStack[subMenuStackPtr];
        bool updated = false;

        if (!gpio_get_level(BTN_UP)) {
            while (!gpio_get_level(BTN_UP)) vTaskDelay(pdMS_TO_TICKS(50));
            currentState->currentSelection = (currentState->currentSelection - 1 + currentState->itemCount) % currentState->itemCount;
            updated = true;
        } else if (!gpio_get_level(BTN_DOWN)) {
            while (!gpio_get_level(BTN_DOWN)) vTaskDelay(pdMS_TO_TICKS(50));
            currentState->currentSelection = (currentState->currentSelection + 1) % currentState->itemCount;
            updated = true;
        } else if (!gpio_get_level(BTN_OK)) { // Lógica diferente aqui
            while (!gpio_get_level(BTN_OK)) vTaskDelay(pdMS_TO_TICKS(800));
            selection_result = currentState->currentSelection; // Guarda o índice
            break; // Sai do loop para retornar o valor
        } else if (!gpio_get_level(BTN_BACK)) {
            while (!gpio_get_level(BTN_BACK)) vTaskDelay(pdMS_TO_TICKS(50));
            selection_result = -1; // Garante que o retorno é -1
            break; // Sai do loop
        }

        if (updated) {
            render_current_submenu();
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    return selection_result;
}
