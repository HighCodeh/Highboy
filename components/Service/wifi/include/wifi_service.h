#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include "esp_err.h"
#include "esp_mac.h"

void wifi_init(void);
void wifi_service_init(void);
void wifi_change_to_hotspot(const char *new_ssid);

#endif // WIFI_SERVICE_H
