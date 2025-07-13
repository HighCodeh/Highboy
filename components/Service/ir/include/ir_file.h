#ifndef IR_FILE_H
#define IR_FILE_H

#include "driver/rmt_common.h"
#include "ir_protocol.h"
#include <stdint.h>
#include <stdbool.h>

// Estruturas existentes...
typedef struct {
    char protocol[16];
    uint32_t address;
    uint32_t command;
    uint16_t bits;
    bool repeat;
} ir_decoded_data_t;

typedef struct {
    uint32_t* data;
    size_t data_count;
    uint32_t frequency;
    float duty_cycle;
} ir_raw_data_t;

// Estrutura para armazenar dados do arquivo .ir
typedef struct {
    char name[64];
    char type[16];  // "parsed" ou "raw"
    char protocol[16];
    uint32_t address;
    uint32_t command;
    uint32_t frequency;
    float duty_cycle;
    uint32_t* raw_data;
    size_t raw_data_count;
} ir_file_data_t;

// Funções existentes
void ir_file_print_parsed(const char* signal_name, const ir_decoded_data_t* decoded);
void ir_file_print_raw(const char* signal_name, const ir_raw_data_t* raw_data);
bool ir_file_symbols_to_decoded(const rmt_symbol_word_t* symbols, size_t symbol_count, ir_decoded_data_t* decoded);
bool ir_file_symbols_to_raw(const rmt_symbol_word_t* symbols, size_t symbol_count, ir_raw_data_t* raw_data);
void ir_file_free_raw_data(ir_raw_data_t* raw_data);
void ir_file_auto_print(const char* signal_name, const rmt_symbol_word_t* symbols, size_t symbol_count);

// NOVAS FUNÇÕES
bool ir_file_parse_string(const char* ir_content, ir_file_data_t* file_data);
bool ir_file_transmit(const ir_file_data_t* file_data, void* encoder);
void ir_file_free_data(ir_file_data_t* file_data);

#endif // IR_FILE_H