#include "sd_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>

static const char *TAG = "SD_MANAGER";
#define MOUNT_POINT "/sdcard"

typedef enum {
    SD_CMD_INIT,
    SD_CMD_SAVE_FILE,
    SD_CMD_READ_FILE,
    SD_CMD_LIST_FILES
} sd_command_t;

typedef struct {
    sd_command_t command;
    SemaphoreHandle_t sync_sem;

    union {
        struct {
            const char* filename;
            const char* content;
            bool result;
        } save_file;

        struct {
            const char* filename;
            char* buffer;
            size_t buffer_size;
            size_t* bytes_read;
            bool result;
        } read_file;

        struct {
            const char* path;
            int* count;
        } list_files;

        struct {
            bool result;
        } init;
    } payload;

    esp_err_t result;
} SDMessage;

static QueueHandle_t sd_queue = NULL;
static sdmmc_card_t* card = NULL;

// ================= SD Thread =================
static void sd_thread(void *pvParameters) {
    for (;;) {
        SDMessage msg;
        if (xQueueReceive(sd_queue, &msg, portMAX_DELAY) == pdPASS) {
            switch (msg.command) {
                case SD_CMD_INIT: {
                    ESP_LOGI(TAG, "Inicializando SD Card...");
                    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
                    host.slot = SPI3_HOST;
                    host.max_freq_khz = 400;

                    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
                    slot_config.gpio_cs = SD_CARD_PIN_CS;
                    slot_config.host_id = host.slot;

                    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
                        .format_if_mount_failed = false,
                        .max_files = 5,
                        .allocation_unit_size = 16 * 1024
                    };

                    esp_err_t ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
                    if (ret != ESP_OK) {
                        ESP_LOGE(TAG, "Falha ao montar SD: %s", esp_err_to_name(ret));
                        msg.result = ESP_FAIL;
                        msg.payload.init.result = false;
                    } else {
                        ESP_LOGI(TAG, "SD Card montado com sucesso!");
                        msg.result = ESP_OK;
                        msg.payload.init.result = true;
                    }
                    break;
                }

                case SD_CMD_SAVE_FILE: {
                    char path[256];
                    snprintf(path, sizeof(path), "%s/%s", MOUNT_POINT, msg.payload.save_file.filename);
                    FILE* f = fopen(path, "w");
                    if (f) {
                        size_t written = fwrite(msg.payload.save_file.content, 1, strlen(msg.payload.save_file.content), f);
                        fflush(f);
                        fclose(f);
                        msg.payload.save_file.result = (written == strlen(msg.payload.save_file.content));
                        msg.result = msg.payload.save_file.result ? ESP_OK : ESP_FAIL;
                    } else {
                        ESP_LOGE(TAG, "Falha ao abrir arquivo: %s (errno %d)", path, errno);
                        msg.payload.save_file.result = false;
                        msg.result = ESP_FAIL;
                    }
                    break;
                }

                case SD_CMD_READ_FILE: {
                    char path[256];
                    snprintf(path, sizeof(path), "%s/%s", MOUNT_POINT, msg.payload.read_file.filename);
                    FILE* f = fopen(path, "r");
                    if (f) {
                        fseek(f, 0, SEEK_END);
                        long size = ftell(f);
                        fseek(f, 0, SEEK_SET);

                        if (size > 0 && size < msg.payload.read_file.buffer_size) {
                            fread(msg.payload.read_file.buffer, 1, size, f);
                            msg.payload.read_file.buffer[size] = '\0';
                            if (msg.payload.read_file.bytes_read)
                                *msg.payload.read_file.bytes_read = size;
                            msg.payload.read_file.result = true;
                            msg.result = ESP_OK;
                        } else {
                            msg.payload.read_file.result = false;
                            msg.result = ESP_FAIL;
                        }
                        fclose(f);
                    } else {
                        ESP_LOGE(TAG, "Falha ao abrir arquivo: %s", path);
                        msg.payload.read_file.result = false;
                        msg.result = ESP_FAIL;
                    }
                    break;
                }

                case SD_CMD_LIST_FILES: {
                    DIR* dir = opendir(msg.payload.list_files.path);
                    if (!dir) {
                        ESP_LOGE(TAG, "Falha ao abrir diretório: %s", msg.payload.list_files.path);
                        msg.result = ESP_FAIL;
                    } else {
                        int count = 0;
                        struct dirent* entry;
                        while ((entry = readdir(dir)) != NULL) {
                            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                                count++;
                                ESP_LOGI(TAG, "Arquivo: %s", entry->d_name);
                            }
                        }
                        closedir(dir);
                        if (msg.payload.list_files.count)
                            *msg.payload.list_files.count = count;
                        msg.result = ESP_OK;
                    }
                    break;
                }
            }

            if (msg.sync_sem) {
                xSemaphoreGive(msg.sync_sem);
            }
        }
    }
}

// ================= API =================
esp_err_t sd_manager_init(void) {
    if (sd_queue) return ESP_OK;

    sd_queue = xQueueCreate(10, sizeof(SDMessage));
    if (!sd_queue) return ESP_FAIL;

    if (xTaskCreate(sd_thread, "sd_thread", 4096, NULL, 5, NULL) != pdPASS) {
        vQueueDelete(sd_queue);
        sd_queue = NULL;
        return ESP_FAIL;
    }

    SDMessage msg;
    msg.command = SD_CMD_INIT;
    msg.sync_sem = xSemaphoreCreateBinary();
    if (!msg.sync_sem) return ESP_FAIL;

    xQueueSend(sd_queue, &msg, pdMS_TO_TICKS(1000));
    xSemaphoreTake(msg.sync_sem, pdMS_TO_TICKS(10000));
    esp_err_t res = msg.result;
    vSemaphoreDelete(msg.sync_sem);
    return res;
}

bool sd_manager_save_file_sync(const char* filename, const char* content) {
    if (!sd_queue) return false;
    SDMessage msg;
    msg.command = SD_CMD_SAVE_FILE;
    msg.sync_sem = xSemaphoreCreateBinary();
    msg.payload.save_file.filename = filename;
    msg.payload.save_file.content = content;

    xQueueSend(sd_queue, &msg, pdMS_TO_TICKS(1000));
    xSemaphoreTake(msg.sync_sem, pdMS_TO_TICKS(5000));
    bool result = msg.payload.save_file.result;
    vSemaphoreDelete(msg.sync_sem);
    return result;
}

size_t sd_manager_read_file_sync(const char* filename, char* buffer, size_t buffer_size) {
    if (!sd_queue) return 0;
    SDMessage msg;
    msg.command = SD_CMD_READ_FILE;
    msg.sync_sem = xSemaphoreCreateBinary();
    msg.payload.read_file.filename = filename;
    msg.payload.read_file.buffer = buffer;
    msg.payload.read_file.buffer_size = buffer_size;
    msg.payload.read_file.bytes_read = NULL;

    xQueueSend(sd_queue, &msg, pdMS_TO_TICKS(1000));
    xSemaphoreTake(msg.sync_sem, pdMS_TO_TICKS(5000));
    bool ok = msg.payload.read_file.result;
    vSemaphoreDelete(msg.sync_sem);
    return ok ? strlen(buffer) : 0;
}

int sd_manager_list_files_sync(const char* path) {
    if (!sd_queue) return -1;
    SDMessage msg;
    msg.command = SD_CMD_LIST_FILES;
    msg.sync_sem = xSemaphoreCreateBinary();
    msg.payload.list_files.path = path;
    int count = 0;
    msg.payload.list_files.count = &count;

    xQueueSend(sd_queue, &msg, pdMS_TO_TICKS(1000));
    xSemaphoreTake(msg.sync_sem, pdMS_TO_TICKS(5000));
    vSemaphoreDelete(msg.sync_sem);
    return count;
}
