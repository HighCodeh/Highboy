#ifndef IR_DECODER_H
#define IR_DECODER_H

#include <stdint.h>
#include <stddef.h>
#include "driver/rmt_rx.h"

// Estrutura para dados decodificados
typedef struct {
    char protocol[32];
    uint32_t address;
    uint32_t command;
} ir_decoded_data_t;

// Funções de decodificação por protocolo
uint32_t decode_nec_data(const rmt_symbol_word_t* symbols, size_t count);
uint32_t decode_sony_sirc(const rmt_symbol_word_t* symbols, size_t count);
uint32_t decode_rc6(const rmt_symbol_word_t* symbols, size_t count);

// Função principal de decodificação
void decode_protocol_data(const char* protocol, const rmt_symbol_word_t* symbols, size_t count, ir_decoded_data_t* decoded);

#endif // IR_DECODER_H