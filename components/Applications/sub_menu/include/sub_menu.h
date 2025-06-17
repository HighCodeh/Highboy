#ifndef SUB_MENU_H
#define SUB_MENU_H

#include <stdint.h>

// === Tipo de função para ação do item de menu ===
typedef void (*MenuAction)(void);

// === Estrutura para representar um item do menu ===
typedef struct {
    const char *label;               // Texto do item
    const uint16_t *icon;            // Ícone bitmap RGB565 (pode ser NULL)
    MenuAction action;               // Função que será chamada ao selecionar
} MenuItem;

// === Funções públicas ===
void show_menu(const MenuItem *items, int itemCount, const char *title);
void show_wifi_menu(void);
void handle_menu_controls(void);

#endif // SUB_MENU_H