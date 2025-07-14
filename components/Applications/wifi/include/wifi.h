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

// Armazena os resultados do último scan

// void wifi_deauther_task(void *pvParameters);
// void wifi_deauther_web_task(void *pvParameters);
// void wifi_deauther_send_deauth_frame(const wifi_ap_record_t *ap_record, deauth_frame_type_t type);
// void wifi_deauther_send_raw_frame(const uint8_t *frame_buffer, int size);
// void wifi_deauther_scan(void);

void show_wifi_submenu(void);

#endif // WIFI__H
