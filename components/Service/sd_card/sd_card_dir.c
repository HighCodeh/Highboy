/**
 * @file sd_card_dir.c
 * @brief Implementação das operações de diretório
 */

#include "sd_card_dir.h"
#include "sd_card_init.h"
#include "sd_card_file.h"
#include "esp_log.h"
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

static const char *TAG = "sd_dir";

#define MAX_PATH_LEN 512

static esp_err_t format_path(const char *path, char *full_path, size_t size)
{
    if (path[0] == '/') {
        snprintf(full_path, size, "%s%s", SD_MOUNT_POINT, path);
    } else {
        snprintf(full_path, size, "%s/%s", SD_MOUNT_POINT, path);
    }
    return ESP_OK;
}

esp_err_t sd_dir_create(const char *path)
{
    if (!sd_is_mounted() || path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[MAX_PATH_LEN];
    format_path(path, full_path, sizeof(full_path));

    if (mkdir(full_path, 0775) != 0) {
        ESP_LOGE(TAG, "Erro ao criar dir: %s", full_path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Diretório criado: %s", full_path);
    return ESP_OK;
}

esp_err_t sd_dir_remove(const char *path)
{
    if (!sd_is_mounted() || path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[MAX_PATH_LEN];
    format_path(path, full_path, sizeof(full_path));

    if (rmdir(full_path) != 0) {
        ESP_LOGE(TAG, "Erro ao remover dir: %s", full_path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Diretório removido: %s", full_path);
    return ESP_OK;
}

esp_err_t sd_dir_remove_recursive(const char *path)
{
    if (!sd_is_mounted() || path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[MAX_PATH_LEN];
    format_path(path, full_path, sizeof(full_path));

    DIR *dir = opendir(full_path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Erro ao abrir dir: %s", full_path);
        return ESP_FAIL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char entry_path[MAX_PATH_LEN];
        int ret = snprintf(entry_path, sizeof(entry_path), "%s/%s", full_path, entry->d_name);
        if (ret >= (int)sizeof(entry_path)) {
            ESP_LOGW(TAG, "Caminho muito longo, ignorando: %s", entry->d_name);
            continue;
        }

        struct stat st;
        if (stat(entry_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                char rel_path[MAX_PATH_LEN];
                ret = snprintf(rel_path, sizeof(rel_path), "%s/%s", path, entry->d_name);
                if (ret < (int)sizeof(rel_path)) {
                    sd_dir_remove_recursive(rel_path);
                }
            } else {
                unlink(entry_path);
            }
        }
    }

    closedir(dir);
    
    if (rmdir(full_path) != 0) {
        ESP_LOGE(TAG, "Erro ao remover dir: %s", full_path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Diretório removido recursivamente: %s", full_path);
    return ESP_OK;
}

bool sd_dir_exists(const char *path)
{
    if (!sd_is_mounted() || path == NULL) {
        return false;
    }

    char full_path[MAX_PATH_LEN];
    format_path(path, full_path, sizeof(full_path));

    struct stat st;
    return (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode));
}

esp_err_t sd_dir_list(const char *path, sd_dir_callback_t callback, void *user_data)
{
    if (!sd_is_mounted() || path == NULL || callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[MAX_PATH_LEN];
    format_path(path, full_path, sizeof(full_path));

    DIR *dir = opendir(full_path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Erro ao abrir dir: %s", full_path);
        return ESP_FAIL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char entry_path[MAX_PATH_LEN];
        int ret = snprintf(entry_path, sizeof(entry_path), "%s/%s", full_path, entry->d_name);
        if (ret >= (int)sizeof(entry_path)) {
            ESP_LOGW(TAG, "Caminho muito longo, ignorando: %s", entry->d_name);
            continue;
        }

        struct stat st;
        bool is_dir = false;
        if (stat(entry_path, &st) == 0) {
            is_dir = S_ISDIR(st.st_mode);
        }

        callback(entry->d_name, is_dir, user_data);
    }

    closedir(dir);
    ESP_LOGD(TAG, "Diretório listado: %s", full_path);
    return ESP_OK;
}

esp_err_t sd_dir_count(const char *path, uint32_t *file_count, uint32_t *dir_count)
{
    if (!sd_is_mounted() || path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[MAX_PATH_LEN];
    format_path(path, full_path, sizeof(full_path));

    DIR *dir = opendir(full_path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Erro ao abrir dir: %s", full_path);
        return ESP_FAIL;
    }

    uint32_t files = 0;
    uint32_t dirs = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char entry_path[MAX_PATH_LEN];
        int ret = snprintf(entry_path, sizeof(entry_path), "%s/%s", full_path, entry->d_name);
        if (ret >= (int)sizeof(entry_path)) {
            continue;
        }

        struct stat st;
        if (stat(entry_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                dirs++;
            } else {
                files++;
            }
        }
    }

    closedir(dir);

    if (file_count != NULL) *file_count = files;
    if (dir_count != NULL) *dir_count = dirs;

    ESP_LOGD(TAG, "Contagem em %s: %lu arquivos, %lu dirs", full_path, files, dirs);
    return ESP_OK;
}

esp_err_t sd_dir_is_empty(const char *path, bool *is_empty)
{
    if (is_empty == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t files, dirs;
    esp_err_t ret = sd_dir_count(path, &files, &dirs);
    if (ret == ESP_OK) {
        *is_empty = (files == 0 && dirs == 0);
    }
    return ret;
}

esp_err_t sd_dir_copy_recursive(const char *src_path, const char *dst_path)
{
    if (!sd_is_mounted() || src_path == NULL || dst_path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Cria diretório destino
    esp_err_t ret = sd_dir_create(dst_path);
    if (ret != ESP_OK && !sd_dir_exists(dst_path)) {
        return ret;
    }

    char src_full[MAX_PATH_LEN];
    format_path(src_path, src_full, sizeof(src_full));

    DIR *dir = opendir(src_full);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Erro ao abrir dir origem: %s", src_full);
        return ESP_FAIL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_entry[MAX_PATH_LEN];
        char dst_entry[MAX_PATH_LEN];
        char src_entry_full[MAX_PATH_LEN];
        
        int r1 = snprintf(src_entry, sizeof(src_entry), "%s/%s", src_path, entry->d_name);
        int r2 = snprintf(dst_entry, sizeof(dst_entry), "%s/%s", dst_path, entry->d_name);
        int r3 = snprintf(src_entry_full, sizeof(src_entry_full), "%s/%s", src_full, entry->d_name);
        
        if (r1 >= (int)sizeof(src_entry) || r2 >= (int)sizeof(dst_entry) || r3 >= (int)sizeof(src_entry_full)) {
            ESP_LOGW(TAG, "Caminho muito longo, ignorando: %s", entry->d_name);
            continue;
        }

        struct stat st;
        if (stat(src_entry_full, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                sd_dir_copy_recursive(src_entry, dst_entry);
            } else {
                sd_file_copy(src_entry, dst_entry);
            }
        }
    }

    closedir(dir);
    ESP_LOGI(TAG, "Diretório copiado: %s -> %s", src_path, dst_path);
    return ESP_OK;
}

esp_err_t sd_dir_rename(const char *old_path, const char *new_path)
{
    if (!sd_is_mounted() || old_path == NULL || new_path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char old_full[MAX_PATH_LEN];
    char new_full[MAX_PATH_LEN];
    format_path(old_path, old_full, sizeof(old_full));
    format_path(new_path, new_full, sizeof(new_full));

    if (rename(old_full, new_full) != 0) {
        ESP_LOGE(TAG, "Erro ao renomear dir: %s -> %s", old_full, new_full);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Diretório renomeado: %s -> %s", old_full, new_full);
    return ESP_OK;
}

esp_err_t sd_dir_get_size(const char *path, uint64_t *total_size)
{
    if (!sd_is_mounted() || path == NULL || total_size == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *total_size = 0;

    char full_path[MAX_PATH_LEN];
    format_path(path, full_path, sizeof(full_path));

    DIR *dir = opendir(full_path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Erro ao abrir dir: %s", full_path);
        return ESP_FAIL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char entry_path[MAX_PATH_LEN];
        int ret = snprintf(entry_path, sizeof(entry_path), "%s/%s", full_path, entry->d_name);
        if (ret >= (int)sizeof(entry_path)) {
            ESP_LOGW(TAG, "Caminho muito longo, ignorando: %s", entry->d_name);
            continue;
        }

        struct stat st;
        if (stat(entry_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                char rel_path[MAX_PATH_LEN];
                ret = snprintf(rel_path, sizeof(rel_path), "%s/%s", path, entry->d_name);
                if (ret < (int)sizeof(rel_path)) {
                    uint64_t subdir_size;
                    if (sd_dir_get_size(rel_path, &subdir_size) == ESP_OK) {
                        *total_size += subdir_size;
                    }
                }
            } else {
                *total_size += st.st_size;
            }
        }
    }

    closedir(dir);
    ESP_LOGD(TAG, "Tamanho total de %s: %llu bytes", full_path, *total_size);
    return ESP_OK;
}