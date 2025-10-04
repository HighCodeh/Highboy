#ifndef IR_STORAGE_H
#define IR_STORAGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Caminho base para armazenamento de arquivos IR
#define IR_STORAGE_BASE_PATH "/sdcard"

// Estrutura para armazenar código IR
typedef struct {
    char protocol[32];   // Nome do protocolo (NEC, RC6, RC5, Samsung32, SIRC)
    uint32_t address;    // Endereço
    uint32_t command;    // Comando
    uint8_t toggle;      // Toggle bit (0xFF = não usado/auto)
    uint8_t bits;        // Número de bits (para Sony: 12, 15 ou 20; 0xFF = não usado)
} ir_code_t;

/**
 * @brief Salva código IR em arquivo (versão simples)
 *
 * @param protocol Nome do protocolo
 * @param command Comando
 * @param address Endereço
 * @param filename Nome do arquivo (sem extensão .ir)
 * @return true em sucesso
 */
bool ir_save(const char* protocol, uint32_t command, uint32_t address, const char* filename);

/**
 * @brief Salva código IR em arquivo (versão estendida com toggle e bits)
 *
 * @param protocol Nome do protocolo
 * @param command Comando
 * @param address Endereço
 * @param toggle Toggle bit (0xFF para não usar/auto)
 * @param filename Nome do arquivo (sem extensão .ir)
 * @return true em sucesso
 */
bool ir_save_ex(const char* protocol, uint32_t command, uint32_t address, uint8_t toggle, const char* filename);

/**
 * @brief Salva código IR com todos os parâmetros (incluindo bits para Sony)
 *
 * @param protocol Nome do protocolo
 * @param command Comando
 * @param address Endereço
 * @param toggle Toggle bit (0xFF para não usar/auto)
 * @param bits Número de bits (para Sony: 12, 15, 20; 0xFF = não usado)
 * @param filename Nome do arquivo (sem extensão .ir)
 * @return true em sucesso
 */
bool ir_save_full(const char* protocol, uint32_t command, uint32_t address,
                  uint8_t toggle, uint8_t bits, const char* filename);

/**
 * @brief Carrega código IR de arquivo
 *
 * @param filename Nome do arquivo (sem extensão .ir)
 * @param code Ponteiro para estrutura onde salvar os dados
 * @return true em sucesso
 */
bool ir_load(const char* filename, ir_code_t* code);

#ifdef __cplusplus
}
#endif

#endif // IR_STORAGE_H