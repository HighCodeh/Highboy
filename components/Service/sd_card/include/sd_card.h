/**
 * @file sd_card.h
 * @brief Driver modularizado para cartão SD via SPI - ESP32-S3
 * @author Seu Nome
 * @date 2024
 */

#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdio.h>
#include <stdbool.h>
#include "esp_err.h"
#include "sdmmc_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                            CONFIGURAÇÕES                                   */
/* ========================================================================== */

#define SD_MOUNT_POINT          "/sdcard"
#define SD_MAX_FILES            10
#define SD_ALLOCATION_UNIT      16 * 1024
#define SD_MAX_FREQ_KHZ         SDMMC_FREQ_DEFAULT

/* Pinos SPI para ESP32-S3 */
#define SD_PIN_MOSI             11
#define SD_PIN_MISO             13
#define SD_PIN_CLK              12
#define SD_PIN_CS               10

/* Limites de tamanho */
#define SD_MAX_PATH_LENGTH      256
#define SD_MAX_LINE_LENGTH      512
#define SD_BUFFER_SIZE          4096

/* ========================================================================== */
/*                            ESTRUTURAS                                      */
/* ========================================================================== */

/**
 * @brief Estrutura com informações do cartão SD
 */
typedef struct {
    char name[16];              // Nome do cartão
    uint32_t capacity_mb;       // Capacidade em MB
    uint32_t sector_size;       // Tamanho do setor
    uint32_t num_sectors;       // Número de setores
    uint8_t card_type;          // Tipo do cartão (MMC, SD, SDHC, etc)
    uint32_t speed;             // Velocidade em kHz
    bool is_mounted;            // Status de montagem
} sd_card_info_t;

/**
 * @brief Estrutura para informações de arquivo
 */
typedef struct {
    char path[SD_MAX_PATH_LENGTH];
    size_t size;
    time_t modified_time;
    bool is_directory;
} sd_file_info_t;

/**
 * @brief Estrutura para estatísticas do sistema de arquivos
 */
typedef struct {
    uint64_t total_bytes;
    uint64_t used_bytes;
    uint64_t free_bytes;
    uint32_t total_files;
    uint32_t total_directories;
} sd_fs_stats_t;

/* ========================================================================== */
/*                     FUNÇÕES DE INICIALIZAÇÃO E CONTROLE                    */
/* ========================================================================== */

/**
 * @brief Inicializa o cartão SD com configuração padrão
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_init(void);

/**
 * @brief Inicializa o cartão SD com configuração customizada
 * @param max_files Número máximo de arquivos abertos simultaneamente
 * @param format_if_failed Formatar se falhar ao montar
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_init_custom(uint8_t max_files, bool format_if_failed);

/**
 * @brief Desmonta e libera recursos do cartão SD
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_deinit(void);

/**
 * @brief Verifica se o cartão está montado
 * @return true se montado, false caso contrário
 */
bool sd_card_is_mounted(void);

/**
 * @brief Remonta o cartão SD
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_remount(void);

/* ========================================================================== */
/*                     FUNÇÕES DE INFORMAÇÃO                                  */
/* ========================================================================== */

/**
 * @brief Obtém informações detalhadas do cartão
 * @param info Ponteiro para estrutura que receberá as informações
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_get_info(sd_card_info_t *info);

/**
 * @brief Imprime informações do cartão no console
 */
void sd_card_print_info(void);

/**
 * @brief Obtém estatísticas do sistema de arquivos
 * @param stats Ponteiro para estrutura que receberá as estatísticas
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_get_fs_stats(sd_fs_stats_t *stats);

/**
 * @brief Obtém espaço livre em bytes
 * @param free_bytes Ponteiro para receber o valor
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_get_free_space(uint64_t *free_bytes);

/**
 * @brief Obtém espaço total em bytes
 * @param total_bytes Ponteiro para receber o valor
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_get_total_space(uint64_t *total_bytes);

/**
 * @brief Calcula percentual de uso do cartão
 * @param percentage Ponteiro para receber o percentual (0-100)
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_get_usage_percentage(float *percentage);

/* ========================================================================== */
/*                     FUNÇÕES DE ESCRITA                                     */
/* ========================================================================== */

/**
 * @brief Escreve texto em um arquivo (sobrescreve)
 * @param path Caminho do arquivo
 * @param data Dados a serem escritos
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_write_file(const char *path, const char *data);

/**
 * @brief Anexa texto ao final de um arquivo
 * @param path Caminho do arquivo
 * @param data Dados a serem anexados
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_append_file(const char *path, const char *data);

/**
 * @brief Escreve dados binários em um arquivo
 * @param path Caminho do arquivo
 * @param data Buffer com os dados
 * @param size Tamanho dos dados em bytes
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_write_binary(const char *path, const void *data, size_t size);

/**
 * @brief Anexa dados binários ao final de um arquivo
 * @param path Caminho do arquivo
 * @param data Buffer com os dados
 * @param size Tamanho dos dados em bytes
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_append_binary(const char *path, const void *data, size_t size);

/**
 * @brief Escreve uma linha de texto com quebra de linha automática
 * @param path Caminho do arquivo
 * @param line Linha a ser escrita
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_write_line(const char *path, const char *line);

/**
 * @brief Anexa uma linha de texto com quebra de linha automática
 * @param path Caminho do arquivo
 * @param line Linha a ser anexada
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_append_line(const char *path, const char *line);

/**
 * @brief Escreve dados formatados no arquivo (como fprintf)
 * @param path Caminho do arquivo
 * @param format String de formato
 * @param ... Argumentos variádicos
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_write_formatted(const char *path, const char *format, ...);

/**
 * @brief Anexa dados formatados no arquivo
 * @param path Caminho do arquivo
 * @param format String de formato
 * @param ... Argumentos variádicos
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_append_formatted(const char *path, const char *format, ...);

/* ========================================================================== */
/*                     FUNÇÕES DE LEITURA                                     */
/* ========================================================================== */

/**
 * @brief Lê todo o conteúdo de um arquivo de texto
 * @param path Caminho do arquivo
 * @param buffer Buffer para receber os dados
 * @param buffer_size Tamanho do buffer
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_read_file(const char *path, char *buffer, size_t buffer_size);

/**
 * @brief Lê dados binários de um arquivo
 * @param path Caminho do arquivo
 * @param buffer Buffer para receber os dados
 * @param size Tamanho a ser lido
 * @param bytes_read Ponteiro para receber quantidade lida
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_read_binary(const char *path, void *buffer, size_t size, size_t *bytes_read);

/**
 * @brief Lê uma linha de um arquivo
 * @param path Caminho do arquivo
 * @param buffer Buffer para receber a linha
 * @param buffer_size Tamanho do buffer
 * @param line_number Número da linha a ser lida (começa em 1)
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_read_line(const char *path, char *buffer, size_t buffer_size, uint32_t line_number);

/**
 * @brief Lê todas as linhas de um arquivo
 * @param path Caminho do arquivo
 * @param callback Função callback chamada para cada linha
 * @param user_data Dados do usuário passados para o callback
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_read_lines(const char *path, void (*callback)(const char *line, void *user_data), void *user_data);

/**
 * @brief Conta o número de linhas em um arquivo
 * @param path Caminho do arquivo
 * @param line_count Ponteiro para receber a contagem
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_count_lines(const char *path, uint32_t *line_count);

/**
 * @brief Lê um arquivo em blocos
 * @param path Caminho do arquivo
 * @param offset Offset inicial
 * @param buffer Buffer para receber os dados
 * @param size Tamanho a ser lido
 * @param bytes_read Ponteiro para receber quantidade lida
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_read_chunk(const char *path, size_t offset, void *buffer, size_t size, size_t *bytes_read);

/* ========================================================================== */
/*                     FUNÇÕES DE GERENCIAMENTO DE ARQUIVOS                   */
/* ========================================================================== */

/**
 * @brief Verifica se um arquivo existe
 * @param path Caminho do arquivo
 * @return true se existe, false caso contrário
 */
bool sd_card_file_exists(const char *path);

/**
 * @brief Deleta um arquivo
 * @param path Caminho do arquivo
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_delete_file(const char *path);

/**
 * @brief Renomeia ou move um arquivo
 * @param old_path Caminho antigo
 * @param new_path Caminho novo
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_rename_file(const char *old_path, const char *new_path);

/**
 * @brief Copia um arquivo
 * @param src_path Caminho origem
 * @param dst_path Caminho destino
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_copy_file(const char *src_path, const char *dst_path);

/**
 * @brief Obtém o tamanho de um arquivo
 * @param path Caminho do arquivo
 * @param size Ponteiro para receber o tamanho
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_get_file_size(const char *path, size_t *size);

/**
 * @brief Obtém informações detalhadas de um arquivo
 * @param path Caminho do arquivo
 * @param info Ponteiro para estrutura de informações
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_get_file_info(const char *path, sd_file_info_t *info);

/**
 * @brief Trunca um arquivo para um tamanho específico
 * @param path Caminho do arquivo
 * @param size Novo tamanho
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_truncate_file(const char *path, size_t size);

/* ========================================================================== */
/*                     FUNÇÕES DE GERENCIAMENTO DE DIRETÓRIOS                 */
/* ========================================================================== */

/**
 * @brief Cria um diretório
 * @param path Caminho do diretório
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_create_dir(const char *path);

/**
 * @brief Remove um diretório (deve estar vazio)
 * @param path Caminho do diretório
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_remove_dir(const char *path);

/**
 * @brief Remove um diretório recursivamente (com todo o conteúdo)
 * @param path Caminho do diretório
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_remove_dir_recursive(const char *path);

/**
 * @brief Verifica se um diretório existe
 * @param path Caminho do diretório
 * @return true se existe, false caso contrário
 */
bool sd_card_dir_exists(const char *path);

/**
 * @brief Lista arquivos em um diretório
 * @param path Caminho do diretório
 * @param callback Função callback chamada para cada arquivo
 * @param user_data Dados do usuário passados para o callback
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_list_dir(const char *path, void (*callback)(const char *name, bool is_dir, void *user_data), void *user_data);

/**
 * @brief Conta arquivos em um diretório
 * @param path Caminho do diretório
 * @param file_count Ponteiro para receber a contagem de arquivos
 * @param dir_count Ponteiro para receber a contagem de diretórios
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_count_dir_contents(const char *path, uint32_t *file_count, uint32_t *dir_count);

/* ========================================================================== */
/*                     FUNÇÕES UTILITÁRIAS                                    */
/* ========================================================================== */

/**
 * @brief Formata o caminho completo com o mount point
 * @param path Caminho relativo
 * @param full_path Buffer para receber caminho completo
 * @param size Tamanho do buffer
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_format_path(const char *path, char *full_path, size_t size);

/**
 * @brief Realiza teste de velocidade de escrita
 * @param size_kb Tamanho do teste em KB
 * @param speed_kbps Ponteiro para receber velocidade em KB/s
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_test_write_speed(uint32_t size_kb, float *speed_kbps);

/**
 * @brief Realiza teste de velocidade de leitura
 * @param size_kb Tamanho do teste em KB
 * @param speed_kbps Ponteiro para receber velocidade em KB/s
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_test_read_speed(uint32_t size_kb, float *speed_kbps);

/**
 * @brief Realiza teste completo do cartão SD
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_run_diagnostics(void);

/**
 * @brief Sincroniza dados pendentes no cartão (flush)
 * @return ESP_OK em caso de sucesso
 */
esp_err_t sd_card_sync(void);

#ifdef __cplusplus
}
#endif

#endif /* SD_CARD_H */