// Arquivo: led_ui.h
#ifndef LED_UI_H
#define LED_UI_H

/**
 * @brief Mostra a tela de configuração do LED RGB.
 *
 * Esta função é bloqueante e assume o controle do loop principal
 * para lidar com a entrada do usuário e atualizar a tela e o LED.
 * Retorna quando o usuário pressiona o botão VOLTAR.
 */
void show_led_screen(void);

#endif // LED_UI_H