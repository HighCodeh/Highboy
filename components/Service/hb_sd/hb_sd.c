// hb_sd.c - Gerenciamento de SD completo estilo Flipper Zero
#include "hb_sd.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define TAG "HB_SD"

#define PIN_NUM_MISO  13
#define PIN_NUM_MOSI  11
#define PIN_NUM_CLK   12
#define PIN_NUM_CS    14
#define MOUNT_POINT "/sdcard"

static sdmmc_card_t *card = NULL;
static bool is_mounted = false;

static const char *default_dirs[] = {
    "/sdcard/apps",
    "/sdcard/assets",
    "/sdcard/badusb",
    "/sdcard/favorites",
    "/sdcard/gpio",
    "/sdcard/infrared",
    "/sdcard/logs",
    "/sdcard/nfc",
    "/sdcard/rfid",
    "/sdcard/scripts",
    "/sdcard/subghz",
    "/sdcard/system",
    "/sdcard/tmp",
    "/sdcard/update"
};


esp_err_t sdcard_init(void) {
    if (is_mounted) return ESP_OK;

    static bool spi3_initialized = false;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;
    host.max_freq_khz = 10000;

    if (!spi3_initialized) {
        spi_bus_config_t bus_cfg = {
            .mosi_io_num = PIN_NUM_MOSI,
            .miso_io_num = PIN_NUM_MISO,
            .sclk_io_num = PIN_NUM_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 4096,
        };

        esp_err_t err = spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "SPI3 init failed: %s", esp_err_to_name(err));
            return err;
        }
        spi3_initialized = true;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 16 * 1024,
    };

    esp_err_t ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Mount failed, retrying with lower freq...");
        host.max_freq_khz = 4000;
        ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Mount error: %s", esp_err_to_name(ret));
        return ret;
    }

    is_mounted = true;
    ESP_LOGI(TAG, "SD mounted successfully.");
    sdcard_print_info();
    sdcard_create_default_dirs();
    return ESP_OK;
}

void sdcard_create_default_dirs(void) {
    ESP_LOGI(TAG, "Verificando estrutura de diretórios...");

    for (int i = 0; i < sizeof(default_dirs)/sizeof(default_dirs[0]); i++) {
        const char *dir = default_dirs[i];
        if (sdcard_file_exists(dir)) {
            ESP_LOGI(TAG, "✓ Diretório já existe: %s", dir);
        } else {
            if (sdcard_create_dir(dir) == ESP_OK) {
                ESP_LOGI(TAG, "📁 Diretório criado: %s", dir);
            } else {
                ESP_LOGE(TAG, "⚠️ Falha ao criar: %s", dir);
            }
        }
    }

    ESP_LOGI(TAG, "Estrutura de diretórios verificada.");
}


void sdcard_unmount(void) {
    if (is_mounted && card) {
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
        is_mounted = false;
        ESP_LOGI(TAG, "SD unmounted.");
    }
}

bool sdcard_is_mounted(void) {
    return is_mounted;
}

void sdcard_print_info(void) {
    if (card) {
        sdmmc_card_print_info(stdout, card);
    } else {
        ESP_LOGW(TAG, "No card info available.");
    }
}

sdmmc_card_t *hb_sd_get_card(void) {
    return card;
}

bool sdcard_file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}


esp_err_t sdcard_create_dir(const char *path) {
    if (mkdir(path, 0777) == 0 || errno == EEXIST) {
        return ESP_OK;
    }
    ESP_LOGE(TAG, "mkdir failed: %s", path);
    return ESP_FAIL;
}

esp_err_t sdcard_list_dir(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        ESP_LOGE(TAG, "Falha ao abrir diretório: %s", path);
        return ESP_FAIL;
    }

    struct dirent *entry;
    char full_path[512];

    ESP_LOGI(TAG, "Conteúdo de: %s", path);

    while ((entry = readdir(dir)) != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) {
            ESP_LOGW(TAG, "Não foi possível obter info de: %s", full_path);
            continue;
        }

        char tipo[8];
        if (entry->d_type == DT_DIR)
            strcpy(tipo, "[DIR]");
        else
            strcpy(tipo, "[FILE]");

        char data[32] = "";
        struct tm *tm_info = localtime(&st.st_mtime);
        if (tm_info) {
            strftime(data, sizeof(data), "%Y-%m-%d %H:%M", tm_info);
        }

        ESP_LOGI(TAG, "%s %-20s %8ld bytes %s", tipo, entry->d_name, st.st_size, data);
    }

    closedir(dir);
    return ESP_OK;
}

esp_err_t sdcard_write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) return ESP_FAIL;
    fputs(content, f);
    fclose(f);
    return ESP_OK;
}

esp_err_t sdcard_append_to_file(const char *path, const char *data) {
    FILE *f = fopen(path, "a");
    if (!f) return ESP_FAIL;
    fputs(data, f);
    fclose(f);
    return ESP_OK;
}

esp_err_t sdcard_read_file_to_buffer(const char *path, char *buffer, size_t max_len) {
    FILE *f = fopen(path, "r");
    if (!f) return ESP_FAIL;
    size_t read = fread(buffer, 1, max_len - 1, f);
    buffer[read] = '\0';
    fclose(f);
    return ESP_OK;
}

esp_err_t sdcard_delete_file(const char *path) {
    return remove(path) == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t sdcard_rename_file(const char *old_path, const char *new_path) {
    return rename(old_path, new_path) == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t sdcard_copy_file(const char *src_path, const char *dest_path) {
    FILE *src = fopen(src_path, "r");
    if (!src) return ESP_FAIL;
    FILE *dst = fopen(dest_path, "w");
    if (!dst) {
        fclose(src);
        return ESP_FAIL;
    }
    char buf[256];
    size_t len;
    while ((len = fread(buf, 1, sizeof(buf), src)) > 0) {
        fwrite(buf, 1, len, dst);
    }
    fclose(src);
    fclose(dst);
    return ESP_OK;
}

esp_err_t sdcard_open_txt_file(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) return ESP_FAIL;
    char line[128];
    while (fgets(line, sizeof(line), file)) {
        ESP_LOGI(TAG, "%s", line);
    }
    fclose(file);
    return ESP_OK;
}
