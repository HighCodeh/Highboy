#include "ir_path.h"

static const char *TAG = "IR_PATH";
static bool is_initialized = false;
static sdmmc_card_t *card = NULL;

// Configuração dos pinos SPI para SD Card
#define PIN_NUM_MISO 13
#define PIN_NUM_CLK  12
#define PIN_NUM_MOSI 11
#define PIN_NUM_CS   14

static ir_path_err_t build_full_path(const char* filename, char* full_path, size_t max_len) {
    if (!filename || !full_path) {
        return IR_PATH_ERR_INVALID_PARAM;
    }
    
    // Se o filename já começa com "/ir", usa ele direto
    if (strncmp(filename, IR_PATH, strlen(IR_PATH)) == 0) {
        int ret = snprintf(full_path, max_len, "%s", filename);
        if (ret >= max_len || ret < 0) {
            return IR_PATH_ERR_PATH_TOO_LONG;
        }
    } else {
        // Caso contrário, constrói o path completo
        int ret = snprintf(full_path, max_len, "%s/%s", IR_PATH, filename);
        if (ret >= max_len || ret < 0) {
            return IR_PATH_ERR_PATH_TOO_LONG;
        }
    }
    
    return IR_PATH_OK;
}

static ir_path_err_t safe_path_join(char* dest, size_t dest_size, const char* path1, const char* path2) {
    if (!dest || !path1 || !path2) {
        return IR_PATH_ERR_INVALID_PARAM;
    }
    
    int ret = snprintf(dest, dest_size, "%s/%s", path1, path2);
    if (ret >= dest_size || ret < 0) {
        return IR_PATH_ERR_PATH_TOO_LONG;
    }
    
    return IR_PATH_OK;
}

ir_path_err_t ir_path_init(void) {
    if (is_initialized) {
        ESP_LOGW(TAG, "SD card already initialized");
        return IR_PATH_OK;
    }
    
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Initializing SD card...");
    
    // Configuração do host SPI
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;
    
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus: %s", esp_err_to_name(ret));
        return IR_PATH_ERR_INIT;
    }
    
    // Configuração do slot SPI
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;
    
    // Configuração de montagem
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    // Monta o sistema de arquivos diretamente no ponto "/ir"
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SD card: %s", esp_err_to_name(ret));
        }
        spi_bus_free(host.slot);
        return IR_PATH_ERR_MOUNT;
    }
    
    is_initialized = true;
    ESP_LOGI(TAG, "SD card initialized successfully, mounted at %s", MOUNT_POINT);
    
    // Imprime informações do cartão
    sdmmc_card_print_info(stdout, card);
    
    return IR_PATH_OK;
}

ir_path_err_t ir_path_deinit(void) {
    if (!is_initialized) {
        return IR_PATH_OK;
    }
    
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
        return IR_PATH_ERR_MOUNT;
    }
    
    spi_bus_free(SPI2_HOST);
    is_initialized = false;
    card = NULL;
    
    ESP_LOGI(TAG, "SD card deinitialized");
    return IR_PATH_OK;
}

ir_path_err_t ir_path_write_file(const char* filename, const char* content, bool append) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD card not initialized");
        return IR_PATH_ERR_INIT;
    }
    
    if (!filename || !content) {
        ESP_LOGE(TAG, "Invalid parameters for write_file");
        return IR_PATH_ERR_INVALID_PARAM;
    }
    
    char full_path[MAX_PATH_LEN];
    ir_path_err_t err = build_full_path(filename, full_path, sizeof(full_path));
    if (err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Path too long: %s", filename);
        return err;
    }
    
    const char* mode = append ? "a" : "w";
    FILE* f = fopen(full_path, mode);
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s (errno: %d)", full_path, errno);
        return IR_PATH_ERR_WRITE_FAILED;
    }
    
    size_t content_len = strlen(content);
    size_t written = fwrite(content, 1, content_len, f);
    fclose(f);
    
    if (written != content_len) {
        ESP_LOGE(TAG, "Failed to write complete content to %s", filename);
        return IR_PATH_ERR_WRITE_FAILED;
    }
    
    ESP_LOGI(TAG, "File written successfully: %s (%d bytes)", filename, written);
    return IR_PATH_OK;
}

ir_path_err_t ir_path_read_file(const char* filename, char* buffer, size_t buffer_size) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD card not initialized");
        return IR_PATH_ERR_INIT;
    }
    
    if (!filename || !buffer || buffer_size == 0) {
        ESP_LOGE(TAG, "Invalid parameters for read_file");
        return IR_PATH_ERR_INVALID_PARAM;
    }
    
    char full_path[MAX_PATH_LEN];
    ir_path_err_t err = build_full_path(filename, full_path, sizeof(full_path));
    if (err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Path too long: %s", filename);
        return err;
    }
    
    FILE* f = fopen(full_path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s (errno: %d)", full_path, errno);
        return IR_PATH_ERR_FILE_NOT_FOUND;
    }
    
    size_t read_bytes = fread(buffer, 1, buffer_size - 1, f);
    fclose(f);
    
    buffer[read_bytes] = '\0'; // Null terminate
    
    ESP_LOGI(TAG, "File read successfully: %s (%d bytes)", filename, read_bytes);
    return IR_PATH_OK;
}

ir_path_err_t ir_path_delete_file(const char* filename) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD card not initialized");
        return IR_PATH_ERR_INIT;
    }
    
    if (!filename) {
        ESP_LOGE(TAG, "Invalid filename parameter");
        return IR_PATH_ERR_INVALID_PARAM;
    }
    
    char full_path[MAX_PATH_LEN];
    ir_path_err_t err = build_full_path(filename, full_path, sizeof(full_path));
    if (err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Path too long: %s", filename);
        return err;
    }
    
    if (unlink(full_path) != 0) {
        ESP_LOGE(TAG, "Failed to delete file: %s (errno: %d)", full_path, errno);
        return IR_PATH_ERR_DELETE_FAILED;
    }
    
    ESP_LOGI(TAG, "File deleted successfully: %s", filename);
    return IR_PATH_OK;
}

bool ir_path_file_exists(const char* filename) {
    if (!is_initialized || !filename) {
        return false;
    }
    
    char full_path[MAX_PATH_LEN];
    if (build_full_path(filename, full_path, sizeof(full_path)) != IR_PATH_OK) {
        return false;
    }
    
    struct stat st;
    return (stat(full_path, &st) == 0 && S_ISREG(st.st_mode));
}

ir_path_err_t ir_path_get_file_size(const char* filename, size_t* size) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD card not initialized");
        return IR_PATH_ERR_INIT;
    }
    
    if (!filename || !size) {
        ESP_LOGE(TAG, "Invalid parameters for get_file_size");
        return IR_PATH_ERR_INVALID_PARAM;
    }
    
    char full_path[MAX_PATH_LEN];
    ir_path_err_t err = build_full_path(filename, full_path, sizeof(full_path));
    if (err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Path too long: %s", filename);
        return err;
    }
    
    struct stat st;
    if (stat(full_path, &st) != 0) {
        ESP_LOGE(TAG, "File not found: %s", filename);
        return IR_PATH_ERR_FILE_NOT_FOUND;
    }
    
    *size = st.st_size;
    return IR_PATH_OK;
}

ir_path_err_t ir_path_list_files(file_list_t* file_list) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD card not initialized");
        return IR_PATH_ERR_INIT;
    }
    
    if (!file_list) {
        ESP_LOGE(TAG, "Invalid file_list parameter");
        return IR_PATH_ERR_INVALID_PARAM;
    }
    
    ESP_LOGD(TAG, "Opening directory: %s", IR_PATH);
    DIR* dir = opendir(IR_PATH);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open ir directory: %s (errno: %d)", IR_PATH, errno);
        return IR_PATH_ERR_READ_FAILED;
    }
    
    file_list->count = 0;
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != NULL && file_list->count < MAX_FILES_LIST) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char full_path[MAX_PATH_LEN];
        ir_path_err_t err = safe_path_join(full_path, sizeof(full_path), IR_PATH, entry->d_name);
        if (err != IR_PATH_OK) {
            ESP_LOGW(TAG, "Path too long, skipping: %s", entry->d_name);
            continue;
        }
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            strncpy(file_list->files[file_list->count].name, entry->d_name, MAX_FILENAME_LEN - 1);
            file_list->files[file_list->count].name[MAX_FILENAME_LEN - 1] = '\0';
            file_list->files[file_list->count].size = st.st_size;
            file_list->files[file_list->count].is_directory = S_ISDIR(st.st_mode);
            file_list->count++;
        } else {
            ESP_LOGW(TAG, "Failed to stat: %s (errno: %d)", full_path, errno);
        }
    }
    
    closedir(dir);
    ESP_LOGI(TAG, "Listed %d files/directories", file_list->count);
    return IR_PATH_OK;
}

ir_path_err_t ir_path_create_directory(const char* dirname) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD card not initialized");
        return IR_PATH_ERR_INIT;
    }
    
    if (!dirname) {
        ESP_LOGE(TAG, "Invalid dirname parameter");
        return IR_PATH_ERR_INVALID_PARAM;
    }
    
    char full_path[MAX_PATH_LEN];
    ir_path_err_t err = build_full_path(dirname, full_path, sizeof(full_path));
    if (err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Path too long: %s", dirname);
        return err;
    }
    
    // Verifica se o diretório já existe
    struct stat st = {0};
    if (stat(full_path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            ESP_LOGW(TAG, "Directory already exists: %s", dirname);
            return IR_PATH_OK;  // Retorna sucesso se já existe
        } else {
            ESP_LOGE(TAG, "Path exists but is not a directory: %s", dirname);
            return IR_PATH_ERR_CREATE_DIR;
        }
    }
    
    if (mkdir(full_path, 0700) != 0) {
        ESP_LOGE(TAG, "Failed to create directory: %s (errno: %d)", full_path, errno);
        return IR_PATH_ERR_CREATE_DIR;
    }
    
    ESP_LOGI(TAG, "Directory created successfully: %s", dirname);
    return IR_PATH_OK;
}

ir_path_err_t ir_path_delete_directory(const char* dirname) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD card not initialized");
        return IR_PATH_ERR_INIT;
    }
    
    if (!dirname) {
        ESP_LOGE(TAG, "Invalid dirname parameter");
        return IR_PATH_ERR_INVALID_PARAM;
    }
    
    char full_path[MAX_PATH_LEN];
    ir_path_err_t err = build_full_path(dirname, full_path, sizeof(full_path));
    if (err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Path too long: %s", dirname);
        return err;
    }
    
    if (rmdir(full_path) != 0) {
        ESP_LOGE(TAG, "Failed to delete directory: %s (errno: %d)", full_path, errno);
        return IR_PATH_ERR_DELETE_FAILED;
    }
    
    ESP_LOGI(TAG, "Directory deleted successfully: %s", dirname);
    return IR_PATH_OK;
}

ir_path_err_t ir_path_delete_directory_recursive(const char* dirname) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD card not initialized");
        return IR_PATH_ERR_INIT;
    }
    
    if (!dirname) {
        ESP_LOGE(TAG, "Invalid dirname parameter");
        return IR_PATH_ERR_INVALID_PARAM;
    }
    
    char full_path[MAX_PATH_LEN];
    ir_path_err_t err = build_full_path(dirname, full_path, sizeof(full_path));
    if (err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Path too long: %s", dirname);
        return err;
    }
    
    DIR* dir = opendir(full_path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory for recursive delete: %s", full_path);
        return IR_PATH_ERR_READ_FAILED;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char entry_path[MAX_PATH_LEN];
        err = safe_path_join(entry_path, sizeof(entry_path), full_path, entry->d_name);
        if (err != IR_PATH_OK) {
            ESP_LOGW(TAG, "Path too long, skipping: %s/%s", dirname, entry->d_name);
            continue;
        }
        
        struct stat st;
        if (stat(entry_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                char relative_path[MAX_PATH_LEN];
                err = safe_path_join(relative_path, sizeof(relative_path), dirname, entry->d_name);
                if (err == IR_PATH_OK) {
                    ir_path_delete_directory_recursive(relative_path);
                }
            } else {
                if (unlink(entry_path) != 0) {
                    ESP_LOGW(TAG, "Failed to delete file: %s", entry_path);
                }
            }
        }
    }
    
    closedir(dir);
    return ir_path_delete_directory(dirname);
}

bool ir_path_directory_exists(const char* dirname) {
    if (!is_initialized || !dirname) {
        return false;
    }
    
    char full_path[MAX_PATH_LEN];
    if (build_full_path(dirname, full_path, sizeof(full_path)) != IR_PATH_OK) {
        return false;
    }
    
    struct stat st;
    return (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode));
}

ir_path_err_t ir_path_copy_file(const char* src_filename, const char* dst_filename) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD card not initialized");
        return IR_PATH_ERR_INIT;
    }
    
    if (!src_filename || !dst_filename) {
        ESP_LOGE(TAG, "Invalid parameters for copy_file");
        return IR_PATH_ERR_INVALID_PARAM;
    }
    
    char src_path[MAX_PATH_LEN], dst_path[MAX_PATH_LEN];
    ir_path_err_t err;
    
    err = build_full_path(src_filename, src_path, sizeof(src_path));
    if (err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Source path too long: %s", src_filename);
        return err;
    }
    
    err = build_full_path(dst_filename, dst_path, sizeof(dst_path));
    if (err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Destination path too long: %s", dst_filename);
        return err;
    }
    
    FILE* src = fopen(src_path, "rb");
    if (!src) {
        ESP_LOGE(TAG, "Failed to open source file: %s", src_filename);
        return IR_PATH_ERR_FILE_NOT_FOUND;
    }
    
    FILE* dst = fopen(dst_path, "wb");
    if (!dst) {
        ESP_LOGE(TAG, "Failed to open destination file: %s", dst_filename);
        fclose(src);
        return IR_PATH_ERR_WRITE_FAILED;
    }
    
    char buffer[1024];
    size_t bytes;
    
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes, dst) != bytes) {
            ESP_LOGE(TAG, "Failed to write to destination file");
            fclose(src);
            fclose(dst);
            return IR_PATH_ERR_WRITE_FAILED;
        }
    }
    
    fclose(src);
    fclose(dst);
    
    ESP_LOGI(TAG, "File copied successfully: %s -> %s", src_filename, dst_filename);
    return IR_PATH_OK;
}

ir_path_err_t ir_path_rename_file(const char* old_filename, const char* new_filename) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD card not initialized");
        return IR_PATH_ERR_INIT;
    }
    
    if (!old_filename || !new_filename) {
        ESP_LOGE(TAG, "Invalid parameters for rename_file");
        return IR_PATH_ERR_INVALID_PARAM;
    }
    
    char old_path[MAX_PATH_LEN], new_path[MAX_PATH_LEN];
    ir_path_err_t err;
    
    err = build_full_path(old_filename, old_path, sizeof(old_path));
    if (err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Old path too long: %s", old_filename);
        return err;
    }
    
    err = build_full_path(new_filename, new_path, sizeof(new_path));
    if (err != IR_PATH_OK) {
        ESP_LOGE(TAG, "New path too long: %s", new_filename);
        return err;
    }
    
    if (rename(old_path, new_path) != 0) {
        ESP_LOGE(TAG, "Failed to rename file: %s -> %s (errno: %d)", old_filename, new_filename, errno);
        return IR_PATH_ERR_DELETE_FAILED;
    }
    
    ESP_LOGI(TAG, "File renamed successfully: %s -> %s", old_filename, new_filename);
    return IR_PATH_OK;
}

ir_path_err_t ir_path_get_storage_info(uint64_t* total_bytes, uint64_t* used_bytes) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD card not initialized");
        return IR_PATH_ERR_INIT;
    }
    
    if (!total_bytes || !used_bytes) {
        ESP_LOGE(TAG, "Invalid parameters for get_storage_info");
        return IR_PATH_ERR_INVALID_PARAM;
    }
    
    FATFS* fs;
    DWORD free_clusters;
    
    if (f_getfree("0:", &free_clusters, &fs) != FR_OK) {
        ESP_LOGE(TAG, "Failed to get storage info");
        return IR_PATH_ERR_READ_FAILED;
    }
    
    uint64_t total_sectors = (fs->n_fatent - 2) * fs->csize;
    uint64_t free_sectors = free_clusters * fs->csize;
    
    *total_bytes = total_sectors * fs->ssize;
    *used_bytes = *total_bytes - (free_sectors * fs->ssize);
    
    return IR_PATH_OK;
}

const char* ir_path_error_to_string(ir_path_err_t error) {
    switch (error) {
        case IR_PATH_OK: return "Success";
        case IR_PATH_ERR_INIT: return "Initialization failed";
        case IR_PATH_ERR_MOUNT: return "Mount failed";
        case IR_PATH_ERR_CREATE_DIR: return "Create directory failed";
        case IR_PATH_ERR_FILE_NOT_FOUND: return "File not found";
        case IR_PATH_ERR_WRITE_FAILED: return "Write failed";
        case IR_PATH_ERR_READ_FAILED: return "Read failed";
        case IR_PATH_ERR_DELETE_FAILED: return "Delete failed";
        case IR_PATH_ERR_INVALID_PARAM: return "Invalid parameter";
        case IR_PATH_ERR_MEMORY: return "Memory error";
        case IR_PATH_ERR_PATH_TOO_LONG: return "Path too long";
        default: return "Unknown error";
    }
}