/**
 * @file sd_card_file.c
 * @brief Implementação das operações de arquivo - OTIMIZADO PARA STACK
 * @version 2.0 - Stack-Safe
 * @date 2024
 * 
 * CORREÇÕES APLICADAS:
 * - Buffers grandes movidos de stack para heap
 * - Adicionado tratamento de erros de memória
 * - Reduzido tamanho de buffers de cópia
 * - Prevenção de stack overflow
 */

#include "sd_card_file.h"
#include "sd_card_init.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

static const char *TAG = "sd_file";

#define MAX_PATH_LEN 512
#define COPY_BUFFER_SIZE 512  // Reduzido de 1024 para economizar memória

/**
 * @brief Formata caminho completo com mount point
 */
static esp_err_t format_path(const char *path, char *full_path, size_t size)
{
    if (path[0] == '/') {
        snprintf(full_path, size, "%s%s", SD_MOUNT_POINT, path);
    } else {
        snprintf(full_path, size, "%s/%s", SD_MOUNT_POINT, path);
    }
    return ESP_OK;
}

bool sd_file_exists(const char *path)
{
    if (!sd_is_mounted() || path == NULL) {
        return false;
    }

    char full_path[MAX_PATH_LEN];
    format_path(path, full_path, sizeof(full_path));

    struct stat st;
    return (stat(full_path, &st) == 0 && !S_ISDIR(st.st_mode));
}

esp_err_t sd_file_delete(const char *path)
{
    if (!sd_is_mounted() || path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[MAX_PATH_LEN];
    format_path(path, full_path, sizeof(full_path));

    if (unlink(full_path) != 0) {
        ESP_LOGE(TAG, "Erro ao deletar: %s", full_path);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Arquivo deletado: %s", full_path);
    return ESP_OK;
}

esp_err_t sd_file_rename(const char *old_path, const char *new_path)
{
    if (!sd_is_mounted() || old_path == NULL || new_path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char old_full[MAX_PATH_LEN];
    char new_full[MAX_PATH_LEN];
    format_path(old_path, old_full, sizeof(old_full));
    format_path(new_path, new_full, sizeof(new_full));

    if (rename(old_full, new_full) != 0) {
        ESP_LOGE(TAG, "Erro ao renomear: %s -> %s", old_full, new_full);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Arquivo renomeado: %s -> %s", old_full, new_full);
    return ESP_OK;
}

esp_err_t sd_file_copy(const char *src_path, const char *dst_path)
{
    if (!sd_is_mounted() || src_path == NULL || dst_path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char src_full[MAX_PATH_LEN];
    char dst_full[MAX_PATH_LEN];
    format_path(src_path, src_full, sizeof(src_full));
    format_path(dst_path, dst_full, sizeof(dst_full));

    // Abre arquivo origem
    FILE *src = fopen(src_full, "rb");
    if (src == NULL) {
        ESP_LOGE(TAG, "Erro ao abrir origem: %s", src_full);
        return ESP_FAIL;
    }

    // Abre arquivo destino
    FILE *dst = fopen(dst_full, "wb");
    if (dst == NULL) {
        ESP_LOGE(TAG, "Erro ao criar destino: %s", dst_full);
        fclose(src);
        return ESP_FAIL;
    }

    // *** CORREÇÃO: Aloca buffer no HEAP ao invés de STACK ***
    uint8_t *buffer = (uint8_t *)malloc(COPY_BUFFER_SIZE);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Erro ao alocar buffer de cópia (%d bytes)", COPY_BUFFER_SIZE);
        fclose(src);
        fclose(dst);
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_read;
    esp_err_t ret = ESP_OK;

    // Copia dados em blocos
    while ((bytes_read = fread(buffer, 1, COPY_BUFFER_SIZE, src)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dst) != bytes_read) {
            ESP_LOGE(TAG, "Erro ao escrever no destino");
            ret = ESP_FAIL;
            break;
        }
    }

    // Cleanup
    free(buffer);
    fclose(src);
    fclose(dst);

    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "Arquivo copiado: %s -> %s", src_full, dst_full);
    } else {
        // Remove arquivo parcialmente copiado em caso de erro
        unlink(dst_full);
    }
    
    return ret;
}

esp_err_t sd_file_get_size(const char *path, size_t *size)
{
    if (!sd_is_mounted() || path == NULL || size == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[MAX_PATH_LEN];
    format_path(path, full_path, sizeof(full_path));

    struct stat st;
    if (stat(full_path, &st) != 0) {
        ESP_LOGE(TAG, "Erro ao obter info: %s", full_path);
        return ESP_FAIL;
    }

    *size = st.st_size;
    ESP_LOGD(TAG, "Tamanho de %s: %u bytes", full_path, (unsigned int)*size);
    return ESP_OK;
}

esp_err_t sd_file_get_info(const char *path, sd_file_info_t *info)
{
    if (!sd_is_mounted() || path == NULL || info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[MAX_PATH_LEN];
    format_path(path, full_path, sizeof(full_path));

    struct stat st;
    if (stat(full_path, &st) != 0) {
        ESP_LOGE(TAG, "Erro ao obter info: %s", full_path);
        return ESP_FAIL;
    }

    // Preenche estrutura de informações
    strncpy(info->path, path, sizeof(info->path) - 1);
    info->path[sizeof(info->path) - 1] = '\0';
    info->size = st.st_size;
    info->modified_time = st.st_mtime;
    info->is_directory = S_ISDIR(st.st_mode);

    ESP_LOGD(TAG, "Info obtida: %s (%u bytes)", full_path, (unsigned int)info->size);
    return ESP_OK;
}

esp_err_t sd_file_truncate(const char *path, size_t size)
{
    if (!sd_is_mounted() || path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[MAX_PATH_LEN];
    format_path(path, full_path, sizeof(full_path));

    if (truncate(full_path, size) != 0) {
        ESP_LOGE(TAG, "Erro ao truncar: %s", full_path);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Arquivo truncado: %s para %u bytes", full_path, (unsigned int)size);
    return ESP_OK;
}

esp_err_t sd_file_is_empty(const char *path, bool *is_empty)
{
    if (is_empty == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t size;
    esp_err_t ret = sd_file_get_size(path, &size);
    if (ret == ESP_OK) {
        *is_empty = (size == 0);
    }
    return ret;
}

esp_err_t sd_file_move(const char *src_path, const char *dst_path)
{
    // Move é equivalente a rename no filesystem
    return sd_file_rename(src_path, dst_path);
}

esp_err_t sd_file_compare(const char *path1, const char *path2, bool *are_equal)
{
    if (!sd_is_mounted() || path1 == NULL || path2 == NULL || are_equal == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *are_equal = false;

    // Verifica tamanhos primeiro (otimização)
    size_t size1, size2;
    if (sd_file_get_size(path1, &size1) != ESP_OK) {
        return ESP_FAIL;
    }
    if (sd_file_get_size(path2, &size2) != ESP_OK) {
        return ESP_FAIL;
    }

    if (size1 != size2) {
        ESP_LOGD(TAG, "Arquivos diferentes (tamanhos: %u vs %u)", 
                 (unsigned int)size1, (unsigned int)size2);
        return ESP_OK;
    }

    // Se tamanhos iguais, compara conteúdo
    char full_path1[MAX_PATH_LEN];
    char full_path2[MAX_PATH_LEN];
    format_path(path1, full_path1, sizeof(full_path1));
    format_path(path2, full_path2, sizeof(full_path2));

    FILE *f1 = fopen(full_path1, "rb");
    if (f1 == NULL) {
        ESP_LOGE(TAG, "Erro ao abrir: %s", full_path1);
        return ESP_FAIL;
    }

    FILE *f2 = fopen(full_path2, "rb");
    if (f2 == NULL) {
        ESP_LOGE(TAG, "Erro ao abrir: %s", full_path2);
        fclose(f1);
        return ESP_FAIL;
    }

    // *** CORREÇÃO: Aloca buffers no HEAP ao invés de STACK ***
    uint8_t *buffer1 = (uint8_t *)malloc(COPY_BUFFER_SIZE);
    uint8_t *buffer2 = (uint8_t *)malloc(COPY_BUFFER_SIZE);
    
    if (buffer1 == NULL || buffer2 == NULL) {
        ESP_LOGE(TAG, "Erro ao alocar buffers de comparação (%d bytes cada)", 
                 COPY_BUFFER_SIZE);
        if (buffer1) free(buffer1);
        if (buffer2) free(buffer2);
        fclose(f1);
        fclose(f2);
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_read1, bytes_read2;
    bool equal = true;

    // Compara arquivos bloco por bloco
    while (equal && (bytes_read1 = fread(buffer1, 1, COPY_BUFFER_SIZE, f1)) > 0) {
        bytes_read2 = fread(buffer2, 1, COPY_BUFFER_SIZE, f2);
        
        if (bytes_read1 != bytes_read2 || memcmp(buffer1, buffer2, bytes_read1) != 0) {
            equal = false;
        }
    }

    // Cleanup
    free(buffer1);
    free(buffer2);
    fclose(f1);
    fclose(f2);

    *are_equal = equal;
    ESP_LOGD(TAG, "Arquivos %s", equal ? "iguais" : "diferentes");
    return ESP_OK;
}

esp_err_t sd_file_clear(const char *path)
{
    return sd_file_truncate(path, 0);
}

esp_err_t sd_file_get_extension(const char *path, char *extension, size_t size)
{
    if (path == NULL || extension == NULL || size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const char *dot = strrchr(path, '.');
    if (dot == NULL || dot == path) {
        extension[0] = '\0';
        return ESP_OK;
    }

    strncpy(extension, dot + 1, size - 1);
    extension[size - 1] = '\0';

    ESP_LOGD(TAG, "Extensão de %s: %s", path, extension);
    return ESP_OK;
}