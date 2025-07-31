#ifndef STORAGE_API_H
#define STORAGE_API_H

#include "storage_message.h"

/**
 * @brief Lista os arquivos e diretórios no caminho especificado (função bloqueante).
 * @param path O caminho do diretório a ser listado (ex: "/").
 * @param list Ponteiro para a estrutura que receberá a lista de arquivos.
 * @return esp_err_t ESP_OK se sucesso.
 */
esp_err_t storage_list_files(const char *path, file_list_t *list);

/**
 * @brief Lê o conteúdo de um arquivo para um buffer (função bloqueante).
 * @param path Caminho do arquivo a ser lido (ex: "/teste.txt").
 * @param buffer Buffer para armazenar o conteúdo.
 * @param buffer_size Tamanho total do buffer.
 * @param bytes_read Ponteiro para uma variável que receberá o número de bytes lidos.
 * @return esp_err_t ESP_OK se sucesso.
 */
esp_err_t storage_read_file(const char* path, char* buffer, size_t buffer_size, size_t* bytes_read);

esp_err_t storage_write_file(const char* path, const void* data, size_t size, bool append);

#endif // STORAGE_API_H