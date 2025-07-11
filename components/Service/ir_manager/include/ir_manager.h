#ifndef IR_MANAGER_H
#define IR_MANAGER_H

#include "driver/rmt_common.h"
#include "ir_path.h"  // Inclui o módulo de SD Card
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>   // Para va_list

// Constantes
#define MEM_BLOCK_SYMBOLS 64
#define MAX_FILENAME_LEN 256

// Estrutura para dados decodificados (movida da main)
typedef struct {
    char protocol[32];
    uint32_t address;
    uint32_t command;
} ir_decoded_data_t;

// Variáveis globais
extern bool has_manager_init;

// Funções principais
bool ir_manager_init();
bool ir_manager_delete();
size_t ir_manager_receive(rmt_symbol_word_t *raw_symbols);

// Funções de gerenciamento de storage (SD Card)
bool ir_manager_init_storage(void);
void ir_manager_deinit_storage(void);
void ir_manager_list_files(void);

// Funções de manipulação de arquivos .ir
bool ir_manager_read_file(const char* filename, char* buffer, size_t buffer_size);
bool ir_manager_delete_file(const char* filename);
bool ir_manager_copy_file(const char* src_filename, const char* dst_filename);
bool ir_manager_rename_file(const char* old_filename, const char* new_filename);

// Função principal encapsulada
void ir_manager_receive_ir();

// Funções de transmissão
bool ir_manager_transmit_raw(rmt_symbol_word_t *raw_symbols, size_t num_symbols);
bool ir_manager_transmit_by_file_name(const char* signal_name);
bool ir_manager_list_ir_files(void);
bool ir_manager_signal_exists(const char* signal_name);
bool ir_manager_delete_signal(const char* signal_name);
bool ir_manager_copy_signal(const char* src_signal_name, const char* dst_signal_name);
bool ir_manager_rename_signal(const char* old_signal_name, const char* new_signal_name);

// Funções auxiliares internas
void generate_ir_filename(char* filename, size_t max_len, const char* protocol);
void generate_filename_with_timestamp(char* filename, size_t size, const char* protocol);

// Funções de captura de output (internas)
void start_output_capture(void);
char* finish_output_capture(void);
int printf_capture(const char* format, ...);

// Funções de decodificação (movidas da main)
char* generate_ir_file_content(const char* signal_name, const ir_decoded_data_t* decoded);
uint32_t decode_sony_sirc(rmt_symbol_word_t* symbols, size_t count);
uint32_t decode_rc6(rmt_symbol_word_t* symbols, size_t count);
void decode_protocol_data(const char* protocol, rmt_symbol_word_t* symbols, size_t count, ir_decoded_data_t* decoded);

// Funções em desenvolvimento
void ir_manager_send_brute_force();
void ir_manager_send_tvbgone();

#endif // IR_MANAGER_H