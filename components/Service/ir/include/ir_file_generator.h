#ifndef IR_FILE_GENERATOR_H
#define IR_FILE_GENERATOR_H

#include <stdint.h>
#include <stddef.h>
#include "ir_decoder.h"

// Função para gerar o conteúdo do arquivo .ir
char* generate_ir_file_content(const char* signal_name, const ir_decoded_data_t* decoded);

// Função para gerar nome do arquivo baseado no timestamp
void generate_filename(char* filename, size_t size, const char* protocol);

// Função para gerar nome do sinal baseado no protocolo e dados
void generate_signal_name(char* signal_name, size_t size, const char* protocol, const ir_decoded_data_t* decoded);

#endif // IR_FILE_GENERATOR_H