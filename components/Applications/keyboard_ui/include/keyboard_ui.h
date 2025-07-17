#ifndef KEYBOARD_UI_H
#define KEYBOARD_UI_H

#include <stdbool.h>
#include <stdint.h>

// Define o tamanho máximo do buffer de texto que o teclado pode manipular.
#define KEYBOARD_BUFFER_SIZE 64

/**
 * @brief Exibe uma interface de teclado de tela cheia para entrada de texto do usuário.
 *
 * Esta função assume o controle do display e do loop de entrada para permitir que o usuário
 * digite um texto usando os botões físicos. A função bloqueia a execução até que o
 * usuário salve ou cancele a entrada.
 *
 * @param buffer Ponteiro para o buffer de char onde o texto digitado será armazenado.
 * Pode conter um texto inicial a ser editado.
 * @param buffer_size O tamanho total do buffer fornecido.
 * @param prompt Um texto/título opcional a ser exibido acima do campo de texto (pode ser NULL).
 * @return true se o usuário salvou o texto (pressionando a tecla "SAV").
 * @return false se o usuário cancelou a operação (pressionando BTN_BACK em um campo vazio, por exemplo).
 */
bool show_keyboard_ui(char* buffer, int buffer_size, const char* prompt);

#endif // KEYBOARD_UI_H