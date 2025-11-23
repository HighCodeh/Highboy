#ifndef SD_CARD_INIT_H
#define SD_CARD_INIT_H

#include "esp_err.h"
#include "sdmmc_cmd.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Configurações gerais */
#define SD_MOUNT_POINT          "/sdcard"
#define SD_MAX_FILES            10
#define SD_ALLOCATION_UNIT      (16 * 1024)

/**
 * @brief Inicializa o cartão SD com configuração padrão
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_init(void);

/**
 * @brief Inicializa com configuração customizada
 * @param max_files Número máximo de arquivos abertos
 * @param format_if_failed Se deve formatar em caso de falha
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_init_custom(uint8_t max_files, bool format_if_failed);

/**
 * @brief Desmonta o cartão SD
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_deinit(void);

/**
 * @brief Verifica se está montado
 * @return true se montado
 */
bool sd_is_mounted(void);

/**
 * @brief Remonta o cartão
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_remount(void);

/**
 * @brief Verifica saúde da conexão
 * @return ESP_OK se saudável
 */
esp_err_t sd_check_health(void);

/**
 * @brief Obtém handle do cartão
 * @return Ponteiro para sdmmc_card_t ou NULL
 */
sdmmc_card_t* sd_get_card_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* SD_CARD_INIT_H */