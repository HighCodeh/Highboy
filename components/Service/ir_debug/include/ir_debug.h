#ifndef IR_DEBUG_H
#define IR_DEBUG_H

#include <stdint.h>
#include <stddef.h>
#include "driver/rmt_rx.h"
#include "ir_decoder.h"

// Função para exibir primeiros símbolos para debug
void debug_show_symbols(const rmt_symbol_word_t* symbols, size_t count, size_t max_symbols);

// Função para exibir informações decodificadas
void debug_show_decoded_info(const ir_decoded_data_t* decoded);

// Função para verificar arquivo salvo
void debug_verify_saved_file(const char* filename, const char* original_content);

#endif // IR_DEBUG_H