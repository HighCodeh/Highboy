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


#ifndef BACKLIGHT_H_
#define BACKLIGHT_H_

#include <stdint.h> // Necessário para usar o tipo de dado uint8_t

/**
 * @brief Inicializa o hardware de controlo do backlight (luz de fundo).
 * * Configura o timer e o canal LEDC para gerar um sinal PWM no pino do backlight.
 * Esta função deve ser chamada uma vez durante a inicialização do sistema 
 * (por exemplo, dentro de st7789_init). O brilho inicial é definido como máximo (255).
 */
void backlight_init(void);

/**
 * @brief Define o nível de brilho do backlight.
 * * @param brightness O nível de brilho desejado, num intervalo de 0 (desligado) a 255 (máximo).
 */
void backlight_set_brightness(uint8_t brightness);

/**
 * @brief Obtém o nível de brilho atual do backlight.
 * * @return uint8_t O valor de brilho atual guardado (0-255).
 */
uint8_t backlight_get_brightness(void);

#endif /* BACKLIGHT_H_ */
