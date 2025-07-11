// ir_transmitter.h

#pragma once

#include "driver/rmt_tx.h"
#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==========================
// Inicialização e Controle
// ==========================

// ===============================
// ⚙️ Configurações padrão do transmissor IR
// ===============================

#define TX_GPIO_NUM                 2
#define MEM_BLOCK_SYMBOLS           64
#define RESOLUTION_HZ               (1 * 1000 * 1000) // 1 MHz
#define FREQUENCY_HZ                38000
#define TRANS_QUEUE_DEPTH           4
#define FLAGS_INVERT_OUT            1
#define FLAGS_WITH_DMA              0
#define FLAGS_POLARITY_ACTIVE_LOW   0
#define DUTY_CYCLE                  0.33
#define LOOP_COUNT                  0



/**
 * @brief Inicializa o canal de transmissão RMT com configuração padrão para IR.
 *
 * @return true se inicializado com sucesso, false em caso de erro.
 */
bool rmt_tx_init(void);

/**
 * @brief Habilita o canal de transmissão RMT.
 */
void rmt_tx_enable(void);

/**
 * @brief Desabilita o canal de transmissão RMT.
 */
void rmt_tx_disable(void);

/**
 * @brief Deleta e libera recursos do canal de transmissão RMT.
 *
 * @return true se deletado com sucesso, false em caso de erro.
 */
bool rmt_tx_delete(void);

/**
 * @brief Deleta o encoder utilizado para transmissão.
 *
 * @param encoder Encoder a ser deletado.
 * @return true se deletado com sucesso, false em caso de erro.
 */
bool rmt_tx_delete_encoder(rmt_encoder_handle_t encoder);


// ==========================
// Transmissão de Dados
// ==========================

/**
 * @brief Transmite um conjunto de símbolos IR via RMT.
 *
 * @param encoder Encoder configurado para modulação (ex: raw encoder).
 * @param raw_symbols Vetor de símbolos RMT a transmitir.
 * @param data_size Tamanho do vetor de símbolos.
 * @return true se a transmissão iniciou corretamente, false em caso de erro.
 */
bool rmt_tx_transmit(rmt_encoder_handle_t encoder, const rmt_symbol_word_t *symbols, size_t num_symbols);

#ifdef __cplusplus
}
#endif
