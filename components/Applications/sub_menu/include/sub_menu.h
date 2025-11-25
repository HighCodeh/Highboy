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

#ifndef SUBMENU_H_
#define SUBMENU_H_

#include <stdint.h>
#include <stdbool.h>

// Estrutura única para todos os itens de menu
typedef struct SubMenuItem {
    const char *label;
    const uint16_t *icon;
    void (*action)(void); 
} SubMenuItem;


// --- Constantes de Layout Públicas ---
#define MAX_VISIBLE_ITEMS 4
#define ITEM_HEIGHT         38
#define ITEM_WIDTH          220
#define START_Y             40
#define ITEM_SPACING        12
#define SCROLL_BAR_WIDTH    4
#define DESIGN_PURPLE_COLOR 0x991D


// --- Funções do Sistema de Menu ---

/**
 * @brief Mostra um menu de COMANDOS. Cada item executa uma ação. Não retorna valor.
 */
void show_submenu(const SubMenuItem *items, int itemCount, const char *title);

/**
 * @brief ✨ NOVO: Mostra um menu de SELEÇÃO e retorna o item escolhido.
 * @return O índice do item selecionado, ou -1 se o usuário pressionar 'BACK'.
 */
int show_picker_menu(const SubMenuItem *items, int itemCount, const char *title);


// --- Funções de Desenho Públicas ---
void menu_draw_header(const char *title);
void menu_draw_item(const SubMenuItem *item, int y, bool selected);
void menu_draw_scrollbar(int current_offset, int total_items, int current_selection);


#endif // SUBMENU_H_
