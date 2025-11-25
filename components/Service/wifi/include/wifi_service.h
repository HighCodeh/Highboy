#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include "esp_err.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

#define WIFI_SCAN_LIST_SIZE 20

// inicializacao
void wifi_init(void);
void wifi_service_init(void);

// funçoes de gerenciamento
void wifi_change_to_hotspot(const char *new_ssid);
esp_err_t wifi_service_connect_to_ap(const char *ssid, const char *password);

// funçoes de scan e storage
void wifi_service_scan(void);
uint16_t wifi_service_get_ap_count(void);
wifi_ap_record_t* wifi_service_get_ap_record(uint16_t index);

#endif // WIFI_SERVICE_H
