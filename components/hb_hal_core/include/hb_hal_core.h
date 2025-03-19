#ifndef HB_HAL_CORE_H
#define HB_HAL_CORE_H

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Códigos de status do HAL Core.
 */
typedef enum {
    HB_HAL_OK = 0,           /**< Operação bem-sucedida */
    HB_HAL_ERROR = -1,       /**< Erro genérico */
    HB_HAL_INIT_FAIL = -2,   /**< Falha na inicialização do hardware */
    HB_HAL_CONFIG_ERROR = -3 /**< Erro de configuração */
} hb_hal_status_t;

/**
 * @brief Inicializa o núcleo do sistema.
 *
 * Esta função realiza a configuração inicial dos recursos essenciais
 * do ESP32‑S3, incluindo a inicialização do sistema de log, verificação das
 * características do chip e registro do motivo do último reset.
 *
 * @return hb_hal_status_t HB_HAL_OK se a inicialização ocorrer corretamente,
 *         ou um código de erro caso contrário.
 */
hb_hal_status_t hb_hal_core_init(void);

/**
 * @brief Reinicializa o sistema.
 *
 * Aciona a reinicialização do ESP32‑S3. Em ambiente de produção, a chamada
 * para esp_restart() efetua o reset do hardware.
 *
 * @return hb_hal_status_t HB_HAL_OK se o comando de reboot for emitido com sucesso.
 */
hb_hal_status_t hb_hal_core_reboot(void);

/**
 * @brief Retorna uma string com o motivo do último reset.
 *
 * A função mapeia o valor retornado por esp_reset_reason() para uma mensagem
 * descritiva, facilitando o diagnóstico do comportamento do sistema.
 *
 * @return const char* String descritiva do motivo do reset.
 */
const char* hb_hal_core_get_reset_reason(void);

#endif // HB_HAL_CORE_H
