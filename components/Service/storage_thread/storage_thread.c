#include "storage_thread.h"
#include "storage_message.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_master.h"
#include "pin_def.h"
#include <dirent.h>

static const char *TAG = "STORAGE_THREAD";
#define MOUNT_POINT "/sdcard"

QueueHandle_t storage_queue;

static void storage_task(void *pvParameters) {
    esp_err_t ret;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CARD_PIN_CS;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao montar o cartão SD (%s). O thread será encerrado.", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Cartão SD montado com sucesso em %s", MOUNT_POINT);

    StorageMessage msg;
    for (;;) {
        if (xQueueReceive(storage_queue, &msg, portMAX_DELAY) == pdPASS) {
            ESP_LOGD(TAG, "Recebido comando: %d", msg.command);

            switch (msg.command) {
                case STORAGE_CMD_LIST_FILES: {
                    file_list_t* list = msg.payload.list_files.list;
                    const char* path = msg.payload.list_files.path;
                    list->count = 0;
                    char full_path[256];
                    snprintf(full_path, sizeof(full_path), MOUNT_POINT "%s", path);
                    DIR* dir = opendir(full_path);
                    if (!dir) {
                        ESP_LOGE(TAG, "Falha ao abrir o diretório: %s", full_path);
                        msg.result = ESP_FAIL;
                    } else {
                        struct dirent* entry;
                        while ((entry = readdir(dir)) != NULL && list->count < MAX_FILES) {
                            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
                            strncpy(list->names[list->count], entry->d_name, MAX_FILENAME_LEN - 1);
                            list->is_dir[list->count] = (entry->d_type == DT_DIR);
                            list->count++;
                        }
                        closedir(dir);
                        msg.result = ESP_OK;
                    }
                    break;
                }
                case STORAGE_CMD_READ_FILE: {
                    const char* path = msg.payload.read_file.path;
                    char* buffer = msg.payload.read_file.buffer;
                    size_t buffer_size = msg.payload.read_file.buffer_size;
                    size_t* bytes_read = msg.payload.read_file.bytes_read;
                    memset(buffer, 0, buffer_size);
                    *bytes_read = 0;
                    char full_path[256];
                    snprintf(full_path, sizeof(full_path), MOUNT_POINT "%s", path);
                    FILE* f = fopen(full_path, "r");
                    if (f == NULL) {
                        ESP_LOGE(TAG, "Falha ao abrir o arquivo: %s", full_path);
                        msg.result = ESP_FAIL;
                    } else {
                        *bytes_read = fread(buffer, 1, buffer_size - 1, f);
                        fclose(f);
                        msg.result = ESP_OK;
                        ESP_LOGI(TAG, "Lidos %d bytes de %s", *bytes_read, full_path);
                    }
                    break;
                }
                case STORAGE_CMD_WRITE_FILE: {
                    const char* path = msg.payload.write_file.path;
                    const void* data = msg.payload.write_file.data;
                    size_t size = msg.payload.write_file.size;
                    bool append = msg.payload.write_file.append;
                    char full_path[256];
                    snprintf(full_path, sizeof(full_path), MOUNT_POINT "%s", path);
                    const char* mode = append ? "ab" : "wb";
                    FILE* f = fopen(full_path, mode);
                    if (f == NULL) {
                        ESP_LOGE(TAG, "Falha ao abrir o arquivo para escrita: %s", full_path);
                        msg.result = ESP_FAIL;
                    } else {
                        if (fwrite(data, 1, size, f) != size) {
                            ESP_LOGE(TAG, "Falha ao escrever dados no arquivo: %s", full_path);
                            msg.result = ESP_FAIL;
                        } else {
                            msg.result = ESP_OK;
                            ESP_LOGD(TAG, "Escritos %d bytes em %s", size, full_path);
                        }
                        fclose(f);
                    }
                    break; // <-- CORREÇÃO: Adicionado o break que estava faltando.
                }
                // Adicionar um default case é uma boa prática
                default:
                    ESP_LOGW(TAG, "Comando desconhecido recebido: %d", msg.command);
                    msg.result = ESP_ERR_NOT_SUPPORTED;
                    break;
            } // <-- CORREÇÃO: Esta chave fecha o switch
            
            ESP_LOGD(TAG, "Comando %d finalizado. Liberando semáforo.", msg.command);
            xSemaphoreGive(msg.sync_sem);
        }
    }
}

void storage_thread_init(void) {
    storage_queue = xQueueCreate(10, sizeof(StorageMessage));
    if (storage_queue == NULL) {
        ESP_LOGE(TAG, "Falha ao criar a fila de armazenamento!");
        return;
    }
    xTaskCreate(storage_task, "storage_task", 4096, NULL, 5, NULL);
}