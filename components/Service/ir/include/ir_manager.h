#ifndef IR_MANAGER_H
#define IR_MANAGER_H

#include "driver/rmt_common.h"
#include "ir_path.h"  // Inclui o módulo de SD Card
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>   // Para va_list

bool ir_init();

bool ir_transmit_file();
bool ir_receive_file(const char* filename, rmt_symbol_word_t* raw_symbols);

#endif // IR_MANAGER_H