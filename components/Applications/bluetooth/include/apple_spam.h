#ifndef APPLE_SPAM_H
#define APPLE_SPAM_H

#include "esp_err.h"

/**
 * @brief Estrutura que descreve um tipo de ataque de spam.
 */
typedef struct {
    const char *name; // Nome do ataque para exibição no menu
} SpamType;

/**
 * @brief Obtém o número total de ataques de spam disponíveis.
 *
 * @return int O número de ataques.
 */
int spam_get_attack_count(void);

/**
 * @brief Obtém os detalhes de um ataque de spam específico pelo seu índice.
 *
 * @param index O índice do ataque.
 * @return const SpamType* Um ponteiro para os detalhes do ataque, ou NULL se o índice for inválido.
 */
const SpamType* spam_get_attack_type(int index);

/**
 * @brief Inicia o envio de pacotes de anúncio para um tipo de ataque específico.
 * * @param attack_index O índice do ataque a ser iniciado (da lista obtida por spam_get_attack_type).
 * @return esp_err_t 
 * - ESP_OK: Sucesso
 * - ESP_FAIL: Falha ao configurar ou iniciar o anúncio
 * - ESP_ERR_INVALID_STATE: Se o spam já estiver em execução
 * - ESP_ERR_NOT_FOUND: Se o índice do ataque for inválido
 */
esp_err_t spam_start(int attack_index);

/**
 * @brief Para o envio de pacotes de anúncio de spam.
 *
 * @return esp_err_t 
 * - ESP_OK: Sucesso
 * - ESP_FAIL: Falha ao parar o anúncio
 * - ESP_ERR_INVALID_STATE: Se o spam não estiver em execução
 */
esp_err_t spam_stop(void);

#endif // APPLE_SPAM_H

