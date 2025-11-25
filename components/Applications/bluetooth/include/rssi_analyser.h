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

#ifndef RSSI_ANALYSER_H
#define RSSI_ANALYSER_H

#include "host/ble_hs.h"

// A estrutura BtDevice foi movida para cá para ser compartilhada
// entre o bluetooth_menu.c e o RSSI_analyser.c
typedef struct {
    ble_addr_t addr;
    char name[32];
    int8_t rssi;
    uint8_t mfg_data[32];
    uint8_t mfg_data_len;
    uint8_t adv_type;
    uint8_t uuid16[2];
} BtDevice;

/**
 * @brief Exibe uma tela com um gráfico em tempo real do RSSI de um dispositivo Bluetooth.
 *
 * @param dev Um ponteiro para o dispositivo BtDevice a ser monitorado.
 */
void show_rssi_analyser(const BtDevice *dev);

#endif // RSSI_ANALYSER_H
