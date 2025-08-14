#include "bluetooth_scanner.h"
#include "bluetooth_service.h"
#include "host/ble_gap.h"
#include "host/util/util.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "BLE_SCANNER";

static device_found_callback_t s_device_found_cb = NULL;

static void parse_adv_data(const uint8_t *ad, uint8_t ad_len, discovered_device_t *device) {
    device->name[0] = '\0';
    device->mfg_data_len = 0;
    struct ble_hs_adv_fields fields;
    if (ble_hs_adv_parse_fields(&fields, ad, ad_len) != 0) return;
    if (fields.name != NULL && fields.name_len > 0) {
        size_t len = fields.name_len < MAX_DEVICE_NAME_LEN ? fields.name_len : MAX_DEVICE_NAME_LEN;
        memcpy(device->name, fields.name, len);
        device->name[len] = '\0';
    }
    if (fields.mfg_data != NULL && fields.mfg_data_len > 0) {
        size_t len = fields.mfg_data_len < MAX_MFG_DATA_LEN ? fields.mfg_data_len : MAX_MFG_DATA_LEN;
        memcpy(device->mfg_data, fields.mfg_data, len);
        device->mfg_data_len = len;
    }
}

static int bluetooth_scanner_gap_event(struct ble_gap_event *event, void *arg) {
    if (event->type == BLE_GAP_EVENT_DISC) {
        if (s_device_found_cb != NULL) {
            discovered_device_t device;
            memset(&device, 0, sizeof(device));
            memcpy(&device.addr, &event->disc.addr, sizeof(ble_addr_t));
            device.rssi = event->disc.rssi;
            parse_adv_data(event->disc.data, event->disc.length_data, &device);
            s_device_found_cb(&device);
        }
        return 0;
    }
    if (event->type == BLE_GAP_EVENT_DISC_COMPLETE) {
        ESP_LOGI(TAG, "Escaneamento concluído, razão=%d", event->disc_complete.reason);
    }
    return 0;
}

static esp_err_t scanner_start_internal(uint32_t duration_ms, bool filter_duplicates, device_found_callback_t cb) {
    if (ble_gap_disc_active()) {
        ESP_LOGW(TAG, "Escaneamento já está ativo.");
        return ESP_ERR_INVALID_STATE;
    }
    s_device_found_cb = cb;
    struct ble_gap_disc_params disc_params = {
        .passive = 0,
        .filter_duplicates = filter_duplicates,
        .filter_policy = BLE_HCI_SCAN_FILT_NO_WL,
        .limited = 0,
    };
    uint8_t own_addr_type = bluetooth_service_get_own_addr_type();
    int rc = ble_gap_disc(own_addr_type, duration_ms, &disc_params, bluetooth_scanner_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Erro ao iniciar escaneamento: rc=%d", rc);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t bluetooth_scanner_start(uint32_t duration_ms, device_found_callback_t cb) {
    ESP_LOGI(TAG, "Iniciando escaneamento geral.");
    return scanner_start_internal(duration_ms, true, cb);
}

esp_err_t bluetooth_scanner_start_rssi_monitor(device_found_callback_t cb) {
    ESP_LOGI(TAG, "Iniciando monitoramento de RSSI.");
    return scanner_start_internal(BLE_HS_FOREVER, false, cb);
}

esp_err_t bluetooth_scanner_stop(void) {
    if (!ble_gap_disc_active()) return ESP_OK;
    int rc = ble_gap_disc_cancel();
    if (rc != 0 && rc != BLE_HS_EALREADY) {
        ESP_LOGE(TAG, "Erro ao parar escaneamento: rc=%d", rc);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Escaneamento parado.");
    return ESP_OK;
}

