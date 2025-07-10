#ifndef IR_RECEIVER_H
#define IR_RECEIVER_H

#include "driver/rmt_rx.h"
#include "esp_err.h"
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================
// ⚙️ Configurações padrão do receptor IR
// ===============================

#define RX_GPIO_NUM         1
#define MEM_BLOCK_SYMBOLS   64
#define RESOLUTION_HZ       (1 * 1000 * 1000) // 1 MHz
#define FLAGS_INVERT_IN     1
#define FLAGS_WITH_DMA      0

// ===============================
// 🔧 Inicialização e controle do canal RX
// ===============================

/**
 * @brief Inicializa o canal RMT para recepção IR
 * @return true se sucesso, false em erro
 */
bool rmt_rx_init(void);

/**
 * @brief Habilita o canal RX
 */
void rmt_rx_enable(void);

/**
 * @brief Desabilita o canal RX
 */
void rmt_rx_disable(void);

/**
 * @brief Deleta o canal RX
 * @return true se sucesso, false em erro
 */
bool rmt_rx_delete(void);

// ===============================
// 📥 Controle da fila de recepção
// ===============================

/**
 * @brief Cria a fila para recepção IR (necessária antes de capturar)
 */
bool rmt_rx_receive_start_queue(void);

/**
 * @brief Remove a fila de recepção IR
 */
void rmt_rx_receive_stop_queue(void);

// ===============================
// 📡 Captura de sinal IR
// ===============================

/**
 * @brief Inicia a recepção usando buffer externo
 * 
 * @param raw_symbols_receive ponteiro para armazenar os símbolos
 * @return esp_err_t código de erro do ESP-IDF
 */
esp_err_t rmt_rx_start_receive(rmt_symbol_word_t *raw_symbols_receive, size_t max_symbols);

/**
 * @brief Captura contínua de sinais IR (loop infinito)
 *        Apenas para debug ou modo monitor.
 * 
 * @param raw_symbols_receive ponteiro para armazenar os símbolos
 * @return false em erro de recepção
 */
bool rmt_rx_receive_repeat(rmt_symbol_word_t *raw_symbols_receive);

/**
 * @brief Captura um único sinal IR
 * 
 * @param raw_symbols_receive buffer para armazenar os símbolos
 * @param max_symbols tamanho máximo do buffer
 * @param symbols_received ponteiro opcional para retornar a quantidade recebida
 * @return true se recebeu com sucesso, false se timeout
 */
bool rmt_rx_receive_once(rmt_symbol_word_t *raw_symbols_receive, size_t max_symbols, size_t *symbols_received);

#ifdef __cplusplus
}
#endif

#endif // IR_RECEIVER_H
