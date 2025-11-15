/**
 * @file sd_card_dir.h
 * @brief Operações de gerenciamento de diretórios
 */

#ifndef SD_CARD_DIR_H
#define SD_CARD_DIR_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback para listar diretórios
 */
typedef void (*sd_dir_callback_t)(const char *name, bool is_dir, void *user_data);

/**
 * @brief Cria diretório
 * @param path Caminho do diretório
 * @return ESP_OK em sucesso
 */
esp_err_t sd_dir_create(const char *path);

/**
 * @brief Remove diretório vazio
 * @param path Caminho do diretório
 * @return ESP_OK em sucesso
 */
esp_err_t sd_dir_remove(const char *path);

/**
 * @brief Remove diretório recursivamente
 * @param path Caminho do diretório
 * @return ESP_OK em sucesso
 */
esp_err_t sd_dir_remove_recursive(const char *path);

/**
 * @brief Verifica se diretório existe
 * @param path Caminho do diretório
 * @return true se existe
 */
bool sd_dir_exists(const char *path);

/**
 * @brief Lista conteúdo do diretório
 * @param path Caminho do diretório
 * @param callback Função callback
 * @param user_data Dados do usuário
 * @return ESP_OK em sucesso
 */
esp_err_t sd_dir_list(const char *path, sd_dir_callback_t callback, void *user_data);

/**
 * @brief Conta arquivos e diretórios
 * @param path Caminho do diretório
 * @param file_count Ponteiro para contagem de arquivos
 * @param dir_count Ponteiro para contagem de diretórios
 * @return ESP_OK em sucesso
 */
esp_err_t sd_dir_count(const char *path, uint32_t *file_count, uint32_t *dir_count);

/**
 * @brief Verifica se diretório está vazio
 * @param path Caminho do diretório
 * @param is_empty Ponteiro para resultado
 * @return ESP_OK em sucesso
 */
esp_err_t sd_dir_is_empty(const char *path, bool *is_empty);

/**
 * @brief Copia diretório recursivamente
 * @param src_path Origem
 * @param dst_path Destino
 * @return ESP_OK em sucesso
 */
esp_err_t sd_dir_copy_recursive(const char *src_path, const char *dst_path);

/**
 * @brief Renomeia diretório
 * @param old_path Caminho antigo
 * @param new_path Caminho novo
 * @return ESP_OK em sucesso
 */
esp_err_t sd_dir_rename(const char *old_path, const char *new_path);

/**
 * @brief Calcula tamanho total do diretório
 * @param path Caminho do diretório
 * @param total_size Ponteiro para receber tamanho
 * @return ESP_OK em sucesso
 */
esp_err_t sd_dir_get_size(const char *path, uint64_t *total_size);

#ifdef __cplusplus
}
#endif

#endif /* SD_CARD_DIR_H */