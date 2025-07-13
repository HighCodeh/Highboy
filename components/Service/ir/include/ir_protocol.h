#ifndef IR_PROTOCOL_H
#define IR_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "driver/rmt_types.h"  // Para o tipo rmt_symbol_word_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Detecta o protocolo IR a partir de uma sequência de símbolos RMT.
 *
 * @param symbols       Array de símbolos RMT recebidos.
 * @param symbol_count  Número de símbolos válidos no array.
 *
 * @return Nome do protocolo detectado ou "UNKNOWN".
 */
const char* detect_ir_protocol(const rmt_symbol_word_t* symbols, size_t symbol_count);

/**
 * @brief Decodifica os dados de um sinal no formato NEC.
 *
 * @param symbols       Array de símbolos RMT.
 * @param symbol_count  Número de símbolos válidos.
 *
 * @return Valor de 32 bits representando os dados NEC, ou 0 se inválido.
 */
uint32_t decode_nec_data(const rmt_symbol_word_t* symbols, size_t symbol_count);

/**
 * @brief Analisa em detalhe um sinal IR específico (uso customizado).
 *
 * @param symbols       Array de símbolos RMT.
 * @param symbol_count  Número de símbolos válidos.
 */
void analyze_your_signal(const rmt_symbol_word_t* symbols, size_t symbol_count);

#ifdef __cplusplus
}
#endif

#endif // IR_PROTOCOL_H
