// hb_hal_button.h
#pragma once
#include <stdbool.h>
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// Tipos de botões
typedef enum {
    HB_BUTTON_UP,
    HB_BUTTON_DOWN,
    HB_BUTTON_OK,
    HB_BUTTON_BACK,
    HB_BUTTON_LEFT,
    HB_BUTTON_RIGHT,
    HB_BUTTON_COUNT  // Número total de botões
} hb_button_t;

// Estados
typedef enum {
    HB_BUTTON_STATE_RELEASED,
    HB_BUTTON_STATE_PRESSED,
    HB_BUTTON_STATE_HELD
} hb_button_state_t;

// Inicialização
void hb_button_init(void);

// Atualiza estados (chamar no loop principal)
void hb_button_loop(void);

// Verifica se o botão está pressionado
bool hb_button_is_pressed(hb_button_t button);

// Verifica se o botão está sendo segurado
bool hb_button_is_held(hb_button_t button);

#ifdef __cplusplus
}
#endif