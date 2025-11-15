/**
 * @file sd_card.c
 * @brief Implementação do driver modularizado para cartão SD via SPI
 */

#include "sd_card.h"
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdarg.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "esp_timer.h"

/* ========================================================================== */
/*                            VARIÁVEIS PRIVADAS                              */
/* ========================================================================== */

static const char *TAG = "sd_card";
static sdmmc_card_t *s_card = NULL;
static bool s_is_mounted = false;
static sdmmc_host_t s_host;
static spi_bus_config_t s_bus_cfg;

/* ========================================================================== */
/*                     FUNÇÕES DE INICIALIZAÇÃO E CONTROLE                    */
/* ========================================================================== */

esp_err_t sd_card_init(void)
{
    return sd_card_init_custom(SD_MAX_FILES, false);
}

esp_err_t sd_card_init_custom(uint8_t max_files, bool format_if_failed)
{
    if (s_is_mounted) {
        ESP_LOGW(TAG, "Cartão SD já está montado");
        return ESP_OK;
    }

    esp_err_t ret;
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = format_if_failed,
        .max_files = max_files,
        .allocation_unit_size = SD_ALLOCATION_UNIT
    };

    ESP_LOGI(TAG, "Inicializando cartão SD");

    s_host = SDSPI_HOST_DEFAULT();
    s_host.max_freq_khz = SD_MAX_FREQ_KHZ;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_PIN_CS;
    slot_config.host_id = s_host.slot;

    s_bus_cfg.mosi_io_num = SD_PIN_MOSI;
    s_bus_cfg.miso_io_num = SD_PIN_MISO;
    s_bus_cfg.sclk_io_num = SD_PIN_CLK;
    s_bus_cfg.quadwp_io_num = -1;
    s_bus_cfg.quadhd_io_num = -1;
    s_bus_cfg.max_transfer_sz = 4000;

    ret = spi_bus_initialize(s_host.slot, &s_bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar barramento SPI: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &s_host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Falha ao montar filesystem. Cartão pode precisar ser formatado.");
        } else {
            ESP_LOGE(TAG, "Falha ao inicializar cartão: %s", esp_err_to_name(ret));
        }
        spi_bus_free(s_host.slot);
        return ret;
    }

    s_is_mounted = true;
    ESP_LOGI(TAG, "Sistema de arquivos montado com sucesso");
    
    return ESP_OK;
}

esp_err_t sd_card_deinit(void)
{
    if (!s_is_mounted) {
        ESP_LOGW(TAG, "Cartão SD não está montado");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao desmontar cartão: %s", esp_err_to_name(ret));
        return ret;
    }

    spi_bus_free(s_host.slot);
    s_is_mounted = false;
    s_card = NULL;
    
    ESP_LOGI(TAG, "Cartão SD desmontado");
    return ESP_OK;
}

bool sd_card_is_mounted(void)
{
    return s_is_mounted;
}

esp_err_t sd_card_remount(void)
{
    esp_err_t ret;
    
    if (s_is_mounted) {
        ret = sd_card_deinit();
        if (ret != ESP_OK) {
            return ret;
        }
    }
    
    return sd_card_init();
}

/* ========================================================================== */
/*                     FUNÇÕES DE INFORMAÇÃO                                  */
/* ========================================================================== */

esp_err_t sd_card_get_info(sd_card_info_t *info)
{
    if (!s_is_mounted || s_card == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(info->name, s_card->cid.name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    
    info->capacity_mb = ((uint64_t)s_card->csd.capacity) * s_card->csd.sector_size / (1024 * 1024);
    info->sector_size = s_card->csd.sector_size;
    info->num_sectors = s_card->csd.capacity;
    info->card_type = s_card->ocr;
    info->speed = s_card->max_freq_khz;
    info->is_mounted = s_is_mounted;

    return ESP_OK;
}

void sd_card_print_info(void)
{
    if (!s_is_mounted || s_card == NULL) {
        ESP_LOGE(TAG, "Cartão não montado");
        return;
    }

    sd_card_info_t info;
    if (sd_card_get_info(&info) == ESP_OK) {
        ESP_LOGI(TAG, "========== Informações do Cartão SD ==========");
        ESP_LOGI(TAG, "Nome: %s", info.name);
        ESP_LOGI(TAG, "Capacidade: %lu MB", info.capacity_mb);
        ESP_LOGI(TAG, "Tamanho do setor: %lu bytes", info.sector_size);
        ESP_LOGI(TAG, "Número de setores: %lu", info.num_sectors);
        ESP_LOGI(TAG, "Velocidade: %lu kHz", info.speed);
        ESP_LOGI(TAG, "Status: %s", info.is_mounted ? "Montado" : "Desmontado");
        ESP_LOGI(TAG, "============================================");
    }

    sdmmc_card_print_info(stdout, s_card);
}

esp_err_t sd_card_get_fs_stats(sd_fs_stats_t *stats)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (stats == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    FATFS *fs;
    DWORD fre_clust;
    
    if (f_getfree("0:", &fre_clust, &fs) != FR_OK) {
        return ESP_FAIL;
    }

    uint64_t total_sectors = (fs->n_fatent - 2) * fs->csize;
    uint64_t free_sectors = fre_clust * fs->csize;

    stats->total_bytes = total_sectors * fs->ssize;
    stats->free_bytes = free_sectors * fs->ssize;
    stats->used_bytes = stats->total_bytes - stats->free_bytes;

    return ESP_OK;
}

esp_err_t sd_card_get_free_space(uint64_t *free_bytes)
{
    if (free_bytes == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    sd_fs_stats_t stats;
    esp_err_t ret = sd_card_get_fs_stats(&stats);
    if (ret == ESP_OK) {
        *free_bytes = stats.free_bytes;
    }
    return ret;
}

esp_err_t sd_card_get_total_space(uint64_t *total_bytes)
{
    if (total_bytes == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    sd_fs_stats_t stats;
    esp_err_t ret = sd_card_get_fs_stats(&stats);
    if (ret == ESP_OK) {
        *total_bytes = stats.total_bytes;
    }
    return ret;
}

esp_err_t sd_card_get_usage_percentage(float *percentage)
{
    if (percentage == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    sd_fs_stats_t stats;
    esp_err_t ret = sd_card_get_fs_stats(&stats);
    if (ret == ESP_OK) {
        *percentage = (float)stats.used_bytes / stats.total_bytes * 100.0f;
    }
    return ret;
}

/* ========================================================================== */
/*                     FUNÇÕES DE ESCRITA                                     */
/* ========================================================================== */

esp_err_t sd_card_write_file(const char *path, const char *data)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    FILE *f = fopen(full_path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo para escrita: %s", full_path);
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, strlen(data), f);
    fclose(f);

    if (written != strlen(data)) {
        ESP_LOGE(TAG, "Falha ao escrever dados completos");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Arquivo escrito: %s (%d bytes)", full_path, written);
    return ESP_OK;
}

esp_err_t sd_card_append_file(const char *path, const char *data)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    FILE *f = fopen(full_path, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo para anexar: %s", full_path);
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, strlen(data), f);
    fclose(f);

    if (written != strlen(data)) {
        ESP_LOGE(TAG, "Falha ao anexar dados completos");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Dados anexados: %s (%d bytes)", full_path, written);
    return ESP_OK;
}

esp_err_t sd_card_write_binary(const char *path, const void *data, size_t size)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    FILE *f = fopen(full_path, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo para escrita binária: %s", full_path);
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, size, f);
    fclose(f);

    if (written != size) {
        ESP_LOGE(TAG, "Falha ao escrever dados binários completos");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Dados binários escritos: %s (%d bytes)", full_path, written);
    return ESP_OK;
}

esp_err_t sd_card_append_binary(const char *path, const void *data, size_t size)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    FILE *f = fopen(full_path, "ab");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo para anexar binário: %s", full_path);
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, size, f);
    fclose(f);

    if (written != size) {
        ESP_LOGE(TAG, "Falha ao anexar dados binários completos");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Dados binários anexados: %s (%d bytes)", full_path, written);
    return ESP_OK;
}

esp_err_t sd_card_write_line(const char *path, const char *line)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || line == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    FILE *f = fopen(full_path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo: %s", full_path);
        return ESP_FAIL;
    }

    fprintf(f, "%s\n", line);
    fclose(f);

    ESP_LOGD(TAG, "Linha escrita: %s", full_path);
    return ESP_OK;
}

esp_err_t sd_card_append_line(const char *path, const char *line)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || line == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    FILE *f = fopen(full_path, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo: %s", full_path);
        return ESP_FAIL;
    }

    fprintf(f, "%s\n", line);
    fclose(f);

    ESP_LOGD(TAG, "Linha anexada: %s", full_path);
    return ESP_OK;
}

esp_err_t sd_card_write_formatted(const char *path, const char *format, ...)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || format == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    FILE *f = fopen(full_path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo: %s", full_path);
        return ESP_FAIL;
    }

    va_list args;
    va_start(args, format);
    vfprintf(f, format, args);
    va_end(args);

    fclose(f);

    ESP_LOGD(TAG, "Dados formatados escritos: %s", full_path);
    return ESP_OK;
}

esp_err_t sd_card_append_formatted(const char *path, const char *format, ...)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || format == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    FILE *f = fopen(full_path, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo: %s", full_path);
        return ESP_FAIL;
    }

    va_list args;
    va_start(args, format);
    vfprintf(f, format, args);
    va_end(args);

    fclose(f);

    ESP_LOGD(TAG, "Dados formatados anexados: %s", full_path);
    return ESP_OK;
}

/* ========================================================================== */
/*                     FUNÇÕES DE LEITURA                                     */
/* ========================================================================== */

esp_err_t sd_card_read_file(const char *path, char *buffer, size_t buffer_size)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    FILE *f = fopen(full_path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo para leitura: %s", full_path);
        return ESP_FAIL;
    }

    size_t read_size = fread(buffer, 1, buffer_size - 1, f);
    buffer[read_size] = '\0';
    fclose(f);

    ESP_LOGD(TAG, "Arquivo lido: %s (%d bytes)", full_path, read_size);
    return ESP_OK;
}

esp_err_t sd_card_read_binary(const char *path, void *buffer, size_t size, size_t *bytes_read)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    FILE *f = fopen(full_path, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo para leitura binária: %s", full_path);
        return ESP_FAIL;
    }

    size_t read_size = fread(buffer, 1, size, f);
    fclose(f);

    if (bytes_read != NULL) {
        *bytes_read = read_size;
    }

    ESP_LOGD(TAG, "Dados binários lidos: %s (%d bytes)", full_path, read_size);
    return ESP_OK;
}

esp_err_t sd_card_read_line(const char *path, char *buffer, size_t buffer_size, uint32_t line_number)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || buffer == NULL || line_number == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    FILE *f = fopen(full_path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo: %s", full_path);
        return ESP_FAIL;
    }

    uint32_t current_line = 0;
    while (fgets(buffer, buffer_size, f) != NULL) {
        current_line++;
        if (current_line == line_number) {
            // Remove newline
            char *pos = strchr(buffer, '\n');
            if (pos) *pos = '\0';
            fclose(f);
            ESP_LOGD(TAG, "Linha %lu lida: %s", line_number, full_path);
            return ESP_OK;
        }
    }

    fclose(f);
    ESP_LOGW(TAG, "Linha %lu não encontrada em: %s", line_number, full_path);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t sd_card_read_lines(const char *path, void (*callback)(const char *line, void *user_data), void *user_data)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    FILE *f = fopen(full_path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo: %s", full_path);
        return ESP_FAIL;
    }

    char line[SD_MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), f) != NULL) {
        // Remove newline
        char *pos = strchr(line, '\n');
        if (pos) *pos = '\0';
        callback(line, user_data);
    }

    fclose(f);
    ESP_LOGD(TAG, "Todas as linhas processadas: %s", full_path);
    return ESP_OK;
}

esp_err_t sd_card_count_lines(const char *path, uint32_t *line_count)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || line_count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    FILE *f = fopen(full_path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo: %s", full_path);
        return ESP_FAIL;
    }

    *line_count = 0;
    char line[SD_MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), f) != NULL) {
        (*line_count)++;
    }

    fclose(f);
    ESP_LOGD(TAG, "Contagem de linhas: %lu em %s", *line_count, full_path);
    return ESP_OK;
}

esp_err_t sd_card_read_chunk(const char *path, size_t offset, void *buffer, size_t size, size_t *bytes_read)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    FILE *f = fopen(full_path, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo: %s", full_path);
        return ESP_FAIL;
    }

    if (fseek(f, offset, SEEK_SET) != 0) {
        ESP_LOGE(TAG, "Falha ao posicionar no offset %d", offset);
        fclose(f);
        return ESP_FAIL;
    }

    size_t read_size = fread(buffer, 1, size, f);
    fclose(f);

    if (bytes_read != NULL) {
        *bytes_read = read_size;
    }

    ESP_LOGD(TAG, "Chunk lido: %s (offset: %d, lido: %d bytes)", full_path, offset, read_size);
    return ESP_OK;
}

/* ========================================================================== */
/*                     FUNÇÕES DE GERENCIAMENTO DE ARQUIVOS                   */
/* ========================================================================== */

bool sd_card_file_exists(const char *path)
{
    if (!s_is_mounted || path == NULL) {
        return false;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    struct stat st;
    return (stat(full_path, &st) == 0 && S_ISREG(st.st_mode));
}

esp_err_t sd_card_delete_file(const char *path)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    if (unlink(full_path) != 0) {
        ESP_LOGE(TAG, "Falha ao deletar arquivo: %s", full_path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Arquivo deletado: %s", full_path);
    return ESP_OK;
}

esp_err_t sd_card_rename_file(const char *old_path, const char *new_path)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (old_path == NULL || new_path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char old_full_path[SD_MAX_PATH_LENGTH];
    char new_full_path[SD_MAX_PATH_LENGTH];
    snprintf(old_full_path, sizeof(old_full_path), "%s%s", SD_MOUNT_POINT, old_path);
    snprintf(new_full_path, sizeof(new_full_path), "%s%s", SD_MOUNT_POINT, new_path);

    if (rename(old_full_path, new_full_path) != 0) {
        ESP_LOGE(TAG, "Falha ao renomear arquivo: %s -> %s", old_full_path, new_full_path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Arquivo renomeado: %s -> %s", old_full_path, new_full_path);
    return ESP_OK;
}

esp_err_t sd_card_copy_file(const char *src_path, const char *dst_path)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (src_path == NULL || dst_path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char src_full_path[SD_MAX_PATH_LENGTH];
    char dst_full_path[SD_MAX_PATH_LENGTH];
    snprintf(src_full_path, sizeof(src_full_path), "%s%s", SD_MOUNT_POINT, src_path);
    snprintf(dst_full_path, sizeof(dst_full_path), "%s%s", SD_MOUNT_POINT, dst_path);

    FILE *src = fopen(src_full_path, "rb");
    if (src == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo origem: %s", src_full_path);
        return ESP_FAIL;
    }

    FILE *dst = fopen(dst_full_path, "wb");
    if (dst == NULL) {
        ESP_LOGE(TAG, "Falha ao criar arquivo destino: %s", dst_full_path);
        fclose(src);
        return ESP_FAIL;
    }

    uint8_t buffer[SD_BUFFER_SIZE];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes, dst) != bytes) {
            ESP_LOGE(TAG, "Falha ao escrever no arquivo destino");
            fclose(src);
            fclose(dst);
            return ESP_FAIL;
        }
    }

    fclose(src);
    fclose(dst);

    ESP_LOGI(TAG, "Arquivo copiado: %s -> %s", src_full_path, dst_full_path);
    return ESP_OK;
}

esp_err_t sd_card_get_file_size(const char *path, size_t *size)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || size == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    struct stat st;
    if (stat(full_path, &st) != 0) {
        ESP_LOGE(TAG, "Falha ao obter informações do arquivo: %s", full_path);
        return ESP_FAIL;
    }

    *size = st.st_size;
    ESP_LOGD(TAG, "Tamanho do arquivo %s: %d bytes", full_path, *size);
    return ESP_OK;
}

esp_err_t sd_card_get_file_info(const char *path, sd_file_info_t *info)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    struct stat st;
    if (stat(full_path, &st) != 0) {
        ESP_LOGE(TAG, "Falha ao obter informações: %s", full_path);
        return ESP_FAIL;
    }

    strncpy(info->path, path, sizeof(info->path) - 1);
    info->path[sizeof(info->path) - 1] = '\0';
    info->size = st.st_size;
    info->modified_time = st.st_mtime;
    info->is_directory = S_ISDIR(st.st_mode);

    ESP_LOGD(TAG, "Info obtida: %s (%d bytes)", full_path, info->size);
    return ESP_OK;
}

esp_err_t sd_card_truncate_file(const char *path, size_t size)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    if (truncate(full_path, size) != 0) {
        ESP_LOGE(TAG, "Falha ao truncar arquivo: %s", full_path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Arquivo truncado: %s para %d bytes", full_path, size);
    return ESP_OK;
}

/* ========================================================================== */
/*                     FUNÇÕES DE GERENCIAMENTO DE DIRETÓRIOS                 */
/* ========================================================================== */

esp_err_t sd_card_create_dir(const char *path)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    if (mkdir(full_path, 0775) != 0) {
        ESP_LOGE(TAG, "Falha ao criar diretório: %s", full_path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Diretório criado: %s", full_path);
    return ESP_OK;
}

esp_err_t sd_card_remove_dir(const char *path)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    if (rmdir(full_path) != 0) {
        ESP_LOGE(TAG, "Falha ao remover diretório: %s (pode não estar vazio)", full_path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Diretório removido: %s", full_path);
    return ESP_OK;
}

esp_err_t sd_card_remove_dir_recursive(const char *path)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    DIR *dir = opendir(full_path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir diretório: %s", full_path);
        return ESP_FAIL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char entry_path[SD_MAX_PATH_LENGTH];
        snprintf(entry_path, sizeof(entry_path), "%s/%s", full_path, entry->d_name);

        struct stat st;
        if (stat(entry_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                char relative_path[SD_MAX_PATH_LENGTH];
                snprintf(relative_path, sizeof(relative_path), "%s/%s", path, entry->d_name);
                sd_card_remove_dir_recursive(relative_path);
            } else {
                unlink(entry_path);
            }
        }
    }

    closedir(dir);
    
    if (rmdir(full_path) != 0) {
        ESP_LOGE(TAG, "Falha ao remover diretório: %s", full_path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Diretório removido recursivamente: %s", full_path);
    return ESP_OK;
}

bool sd_card_dir_exists(const char *path)
{
    if (!s_is_mounted || path == NULL) {
        return false;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    struct stat st;
    return (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode));
}

esp_err_t sd_card_list_dir(const char *path, void (*callback)(const char *name, bool is_dir, void *user_data), void *user_data)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL || callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    DIR *dir = opendir(full_path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir diretório: %s", full_path);
        return ESP_FAIL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char entry_path[SD_MAX_PATH_LENGTH];
        snprintf(entry_path, sizeof(entry_path), "%s/%s", full_path, entry->d_name);

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

esp_err_t sd_card_count_dir_contents(const char *path, uint32_t *file_count, uint32_t *dir_count)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);

    DIR *dir = opendir(full_path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir diretório: %s", full_path);
        return ESP_FAIL;
    }

    uint32_t files = 0;
    uint32_t dirs = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char entry_path[SD_MAX_PATH_LENGTH];
        snprintf(entry_path, sizeof(entry_path), "%s/%s", full_path, entry->d_name);

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

    if (file_count != NULL) {
        *file_count = files;
    }
    if (dir_count != NULL) {
        *dir_count = dirs;
    }

    ESP_LOGD(TAG, "Contagem em %s: %lu arquivos, %lu diretórios", full_path, files, dirs);
    return ESP_OK;
}

/* ========================================================================== */
/*                     FUNÇÕES UTILITÁRIAS                                    */
/* ========================================================================== */

esp_err_t sd_card_format_path(const char *path, char *full_path, size_t size)
{
    if (path == NULL || full_path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    snprintf(full_path, size, "%s%s", SD_MOUNT_POINT, path);
    return ESP_OK;
}

esp_err_t sd_card_test_write_speed(uint32_t size_kb, float *speed_kbps)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (speed_kbps == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const char *test_file = "/speed_test.tmp";
    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, test_file);

    uint8_t *buffer = malloc(1024);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Falha ao alocar buffer para teste");
        return ESP_ERR_NO_MEM;
    }

    // Preenche buffer com dados aleatórios
    for (int i = 0; i < 1024; i++) {
        buffer[i] = i & 0xFF;
    }

    FILE *f = fopen(full_path, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao criar arquivo de teste");
        free(buffer);
        return ESP_FAIL;
    }

    int64_t start_time = esp_timer_get_time();

    for (uint32_t i = 0; i < size_kb; i++) {
        if (fwrite(buffer, 1, 1024, f) != 1024) {
            ESP_LOGE(TAG, "Falha ao escrever no teste de velocidade");
            fclose(f);
            free(buffer);
            unlink(full_path);
            return ESP_FAIL;
        }
    }

    int64_t end_time = esp_timer_get_time();
    fclose(f);
    free(buffer);
    unlink(full_path);

    float elapsed_sec = (end_time - start_time) / 1000000.0f;
    *speed_kbps = size_kb / elapsed_sec;

    ESP_LOGI(TAG, "Teste de escrita: %.2f KB/s (%lu KB em %.2f s)", *speed_kbps, size_kb, elapsed_sec);
    return ESP_OK;
}

esp_err_t sd_card_test_read_speed(uint32_t size_kb, float *speed_kbps)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (speed_kbps == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const char *test_file = "/speed_test.tmp";
    char full_path[SD_MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, test_file);

    uint8_t *buffer = malloc(1024);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Falha ao alocar buffer para teste");
        return ESP_ERR_NO_MEM;
    }

    // Cria arquivo de teste
    FILE *f = fopen(full_path, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao criar arquivo de teste");
        free(buffer);
        return ESP_FAIL;
    }

    for (uint32_t i = 0; i < size_kb; i++) {
        fwrite(buffer, 1, 1024, f);
    }
    fclose(f);

    // Testa leitura
    f = fopen(full_path, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo de teste");
        free(buffer);
        unlink(full_path);
        return ESP_FAIL;
    }

    int64_t start_time = esp_timer_get_time();

    for (uint32_t i = 0; i < size_kb; i++) {
        if (fread(buffer, 1, 1024, f) != 1024) {
            break;
        }
    }

    int64_t end_time = esp_timer_get_time();
    fclose(f);
    free(buffer);
    unlink(full_path);

    float elapsed_sec = (end_time - start_time) / 1000000.0f;
    *speed_kbps = size_kb / elapsed_sec;

    ESP_LOGI(TAG, "Teste de leitura: %.2f KB/s (%lu KB em %.2f s)", *speed_kbps, size_kb, elapsed_sec);
    return ESP_OK;
}

esp_err_t sd_card_run_diagnostics(void)
{
    if (!s_is_mounted) {
        ESP_LOGE(TAG, "Cartão SD não está montado");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  DIAGNÓSTICO DO CARTÃO SD");
    ESP_LOGI(TAG, "========================================");

    // Informações do cartão
    sd_card_print_info();

    // Espaço em disco
    sd_fs_stats_t stats;
    if (sd_card_get_fs_stats(&stats) == ESP_OK) {
        ESP_LOGI(TAG, "Espaço total: %llu MB", stats.total_bytes / (1024 * 1024));
        ESP_LOGI(TAG, "Espaço usado: %llu MB", stats.used_bytes / (1024 * 1024));
        ESP_LOGI(TAG, "Espaço livre: %llu MB", stats.free_bytes / (1024 * 1024));
        
        float usage;
        if (sd_card_get_usage_percentage(&usage) == ESP_OK) {
            ESP_LOGI(TAG, "Uso: %.1f%%", usage);
        }
    }

    // Teste de escrita
    float write_speed;
    if (sd_card_test_write_speed(100, &write_speed) == ESP_OK) {
        ESP_LOGI(TAG, "Velocidade de escrita: %.2f KB/s", write_speed);
    }

    // Teste de leitura
    float read_speed;
    if (sd_card_test_read_speed(100, &read_speed) == ESP_OK) {
        ESP_LOGI(TAG, "Velocidade de leitura: %.2f KB/s", read_speed);
    }

    // Teste de arquivo
    const char *test_file = "/diag_test.txt";
    const char *test_data = "Teste de diagnóstico do cartão SD";
    
    if (sd_card_write_file(test_file, test_data) == ESP_OK) {
        ESP_LOGI(TAG, "Teste de escrita: OK");
        
        char buffer[128];
        if (sd_card_read_file(test_file, buffer, sizeof(buffer)) == ESP_OK) {
            if (strcmp(buffer, test_data) == 0) {
                ESP_LOGI(TAG, "Teste de leitura: OK");
            } else {
                ESP_LOGE(TAG, "Teste de leitura: FALHOU (dados incorretos)");
            }
        } else {
            ESP_LOGE(TAG, "Teste de leitura: FALHOU");
        }
        
        sd_card_delete_file(test_file);
    } else {
        ESP_LOGE(TAG, "Teste de escrita: FALHOU");
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  DIAGNÓSTICO CONCLUÍDO");
    ESP_LOGI(TAG, "========================================");

    return ESP_OK;
}

esp_err_t sd_card_sync(void)
{
    if (!s_is_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    sync();
    ESP_LOGD(TAG, "Dados sincronizados com o cartão");
    return ESP_OK;
}