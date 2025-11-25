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
