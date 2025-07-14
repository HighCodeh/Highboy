#ifndef SUB_MENU_H
#define SUB_MENU_H

#include "st7789.h" 
#include <stdint.h>  

/**
 * @brief Estrutura que define um único item de menu.
 */
typedef struct {
    const char *label;          // O texto que será exibido para o item.
    const uint16_t *icon;       // Ponteiro para os dados do ícone (bitmap RGB565).
    void (*action)(void);       // Ponteiro para a função que será executada quando o item for selecionado.
} SubMenuItem;

/**
 * @brief Inicia e exibe um menu na tela.
 *
 * Esta função assume o controle, desenha a interface do menu e gerencia a navegação
 * com os botões até que o usuário pressione o botão de voltar.
 *
 * @param items Um array de estruturas SubMenuItem que compõem o menu.
 * @param itemCount O número de itens no array.
 * @param title O título a ser exibido no cabeçalho do menu.
 */
void show_submenu(const SubMenuItem *items, int itemCount, const char *title);
int show_selection_menu(const SubMenuItem *items, int itemCount, const char *title);

#endif 