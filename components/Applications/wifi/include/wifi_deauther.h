#ifndef WIFI_DEAUTHER_H
#define WIFI_DEAUTHER_H

#include <stdint.h>
#include "esp_wifi_types.h"

#define WIFI_SCAN_LIST_SIZE 15
extern wifi_ap_record_t stored_aps[WIFI_SCAN_LIST_SIZE];
extern uint16_t stored_ap_count;


typedef enum {
    DEAUTH_INVALID_AUTH = 0,
    DEAUTH_INACTIVITY,
    DEAUTH_CLASS3,
    DEAUTH_TYPE_COUNT
} deauth_frame_type_t;


// void wifi_deauther_scan(void);
void wifi_deauther_send_deauth_frame(const wifi_ap_record_t *ap_record, deauth_frame_type_t type);
void wifi_deauther_send_raw_frame(const uint8_t *frame_buffer, int size);

// uint16_t get_stored_ap_count(void);
// const wifi_ap_record_t* get_stored_ap_record(int index);


#endif // !WIFI_DEAUTHER_H
