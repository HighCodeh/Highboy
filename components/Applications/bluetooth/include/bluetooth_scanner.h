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

