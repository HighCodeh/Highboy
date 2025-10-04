#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <stdio.h>
#include <stdbool.h>
#include "esp_err.h"

// Pinos padrão do SD card (GPIO do ESP32-S3)
#define SD_CARD_PIN_MISO 13
#define SD_CARD_PIN_MOSI 11
#define SD_CARD_PIN_CLK  12
#define SD_CARD_PIN_CS   14

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa o módulo SD Card
 * @return ESP_OK se inicializado com sucesso, erro caso contrário
 */
esp_err_t sd_manager_init(void);

/**
 * @brief Salva conteúdo em um arquivo no cartão SD
 * @param filename Nome do arquivo (ex: "dados.txt")
 * @param content Conteúdo a ser salvo
 * @return true se salvo com sucesso
 */
bool sd_manager_save_file(const char* filename, const char* content);

/**
 * @brief Salva dados binários em um arquivo no cartão SD
 */
bool sd_manager_save_binary(const char* filename, const void* data, size_t size);

/**
 * @brief Lê o conteúdo de um arquivo do cartão SD
 * @return Buffer alocado com conteúdo ou NULL em caso de erro.
 *         O usuário deve liberar a memória com free()
 */
char* sd_manager_read_file(const char* filename);

/**
 * @brief Lista arquivos do SD Card 
 * @return quantidade de arquivos ou -1 se erro
 */
int sd_manager_list_files(void);

/**
 * @brief Finaliza o módulo SD Card e libera recursos
 */
void sd_manager_deinit(void);

/**
 * @brief Realiza testes de leitura/escrita no SD Card
 */
bool sd_manager_test(void);

#ifdef __cplusplus
}
#endif

#endif // SD_MANAGER_H
