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