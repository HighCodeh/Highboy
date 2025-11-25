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


#ifndef BLUETOOTH_SERVICE_H
#define BLUETOOTH_SERVICE_H

#include "esp_err.h"
#include "stdint.h"

/**
 * @brief Inicializa todo o serviço Bluetooth, incluindo o controlador e a pilha NimBLE.
 * Esta função é síncrona e só retorna quando a pilha BLE está pronta para uso ou
 * se ocorrer um timeout.
 *
 * @return esp_err_t 
 * - ESP_OK: Sucesso
 * - ESP_FAIL: Falha genérica
 * - ESP_ERR_TIMEOUT: Timeout esperando a sincronização da pilha BLE
 * - Outros erros do ESP-IDF
 */
esp_err_t bluetooth_service_init(void);

/**
 * @brief Para completamente o serviço Bluetooth, desabilitando o controlador e liberando recursos.
 *
 * @return esp_err_t 
 * - ESP_OK: Sucesso
 * - Outros erros do ESP-IDF
 */
esp_err_t bluetooth_service_stop(void);

/**
 * @brief Inicia o anúncio (advertising) conectável padrão do dispositivo.
 * Deve ser chamado após bluetooth_service_init() ter retornado com sucesso.
 *
 * @return esp_err_t 
 * - ESP_OK: Sucesso
 * - ESP_FAIL: Se o serviço não estiver inicializado
 * - Outros erros da pilha NimBLE
 */
esp_err_t bluetooth_service_start_advertising(void);

/**
 * @brief Para o anúncio (advertising) do dispositivo.
 *
 * @return esp_err_t 
 * - ESP_OK: Sucesso
 * - Outros erros da pilha NimBLE
 */
esp_err_t bluetooth_service_stop_advertising(void);

/**
 * @brief Obtém o tipo de endereço próprio do dispositivo (ex: público, aleatório).
 * Este valor é determinado durante a sincronização da pilha BLE.
 * * @return uint8_t O tipo de endereço (conforme definido em ble_hs.h).
 */
uint8_t bluetooth_service_get_own_addr_type(void);

#endif // BLUETOOTH_SERVICE_H

