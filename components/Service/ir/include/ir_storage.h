#ifndef IR_STORAGE_H
#define IR_STORAGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char protocol[32];
    uint32_t address;
    uint32_t command;
} ir_code_t;

bool ir_save(const char* protocol, uint32_t command, uint32_t address, const char* filename);

bool ir_load(const char* filename, ir_code_t* code);

#ifdef __cplusplus
}
#endif

#endif // IR_STORAGE_H