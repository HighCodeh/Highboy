// Ficheiro: brightness_ui.h

#ifndef BRIGHTNESS_UI_H_
#define BRIGHTNESS_UI_H_

/**
 * @brief Exibe a tela de ajuste de brilho e processa a entrada do utilizador.
 *
 * Esta função entra num loop para permitir que o utilizador aumente ou diminua
 * o brilho da tela usando os botões. A função só retorna quando o botão
 * "Voltar" (BTN_BACK) é pressionado.
 */
void show_brightness_screen(void);

#endif /* BRIGHTNESS_UI_H_ */