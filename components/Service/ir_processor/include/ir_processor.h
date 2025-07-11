#ifndef IR_PROCESSOR_H
#define IR_PROCESSOR_H

#include <stdint.h>
#include <stddef.h>
#include "driver/rmt_rx.h"
#include "ir_decoder.h"

// Função principal para processar sinais IR
void process_ir_signal(const rmt_symbol_word_t* symbols, size_t count);

// Função para salvar arquivo .ir
bool save_ir_file(const char* protocol, const ir_decoded_data_t* decoded);

#endif // IR_PROCESSOR_H