#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <stdio.h>
#include <stdbool.h>

// Definições dos pinos
#define PIN_NUM_MISO 13
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK  12
#define PIN_NUM_CS   14

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa o módulo SD Card
 * @return true se inicializado com sucesso, false caso contrário
 */
bool sd_manager_init(void);

/**
 * @brief Salva conteúdo em um arquivo no cartão SD
 * @param filename Nome do arquivo (ex: "dados.txt")
 * @param content Conteúdo a ser salvo no arquivo
 * @return true se salvo com sucesso, false caso contrário
 */
bool sd_manager_save_file(const char* filename, const char* content);

bool sd_manager_save_binary(const char* filename, const void* data, size_t size);

/**
 * @brief Lê o conteúdo de um arquivo do cartão SD
 * @param filename Nome do arquivo a ser lido
 * @return String com o conteúdo do arquivo ou NULL se erro. 
 *         IMPORTANTE: O usuário deve liberar a memória com free() após uso
 */
char* sd_manager_read_file(const char* filename);

/**
 * @brief Finaliza o módulo SD Card e libera recursos
 */
void sd_manager_deinit(void);

bool sd_manager_test(void);

bool sd_manager_force_format(void);

#ifdef __cplusplus
}
#endif

#endif // SD_MANAGER_H