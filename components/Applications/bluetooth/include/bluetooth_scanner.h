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

#ifndef BLUETOOTH_SCANNER_H
#define BLUETOOTH_SCANNER_H

#include "esp_err.h"
#include "host/ble_hs.h" 

#define MAX_DEVICE_NAME_LEN 30
#define MAX_MFG_DATA_LEN 255

typedef struct {
    ble_addr_t addr;
    int8_t rssi;
    char name[MAX_DEVICE_NAME_LEN + 1];
    uint8_t mfg_data[MAX_MFG_DATA_LEN];
    uint8_t mfg_data_len;
} discovered_device_t;

typedef void (*device_found_callback_t)(const discovered_device_t *device);

/**
 * @brief Inicia um escaneamento geral para descobrir múltiplos dispositivos.
 */
esp_err_t bluetooth_scanner_start(uint32_t duration_ms, device_found_callback_t cb);

/**
 * @brief Inicia um monitoramento contínuo de RSSI para um único dispositivo.
 * Desativa o filtro de duplicatas para receber todas as atualizações.
 */
esp_err_t bluetooth_scanner_start_rssi_monitor(device_found_callback_t cb);

/**
 * @brief Para qualquer escaneamento BLE em andamento.
 */
esp_err_t bluetooth_scanner_stop(void);

#endif // BLUETOOTH_SCANNER_H

