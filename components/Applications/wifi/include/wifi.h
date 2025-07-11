#ifndef WIFI_H
#define WIFI_H

#include <stdint.h>
#include "esp_wifi_types.h"
// Estrutura para definir um item de menu, igual à do sub_menu

/**
 * @brief Exibe e gerencia o menu de funções WiFi.
 *
 * Esta função assume o controle do loop principal para exibir o menu,
 * lidar com as entradas do usuário e executar as ações selecionadas.
 */
typedef enum {
    DEAUTH_INVALID_AUTH = 0,
    DEAUTH_INACTIVITY,
    DEAUTH_CLASS3,
    DEAUTH_TYPE_COUNT
} deauth_frame_type_t;

void wifi_deauther_task(void *pvParameters);
void wifi_deauther_web_task(void *pvParameters);
void wifi_deauther_send_deauth_frame(const wifi_ap_record_t *ap_record, deauth_frame_type_t type);
void wifi_deauther_send_raw_frame(const uint8_t *frame_buffer, int size);
void wifi_deauther_scan(void);


void show_wifi_menu(void);

#endif // WIFI__H

