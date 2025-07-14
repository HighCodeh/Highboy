// Arquivo: led_control.h

#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdint.h>
#include "led_strip.h" // Adicione este include

// --- Variáveis Externas ---
// Disponibiliza a alça do LED para outros arquivos
extern led_strip_handle_t led_strip;

// --- Declarações de Funções ---
void led_rgb_init(void);

// NOVO: Função para definir uma cor estática no LED
void led_set_color(uint8_t r, uint8_t g, uint8_t b);

// Funções de piscar
void led_blink_red(void);
void led_blink_green(void);
void led_blink_blue(void);
void led_blink_purple(void);

#endif // LED_CONTROL_H