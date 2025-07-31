#include "storage_api.h"
#include "storage_thread.h"
#include "esp_log.h"

static const char *TAG = "STORAGE_API";

esp_err_t storage_list_files(const char *path, file_list_t *list) {
    if (storage_queue == NULL) {
        ESP_LOGE(TAG, "Sistema de armazenamento não inicializado!");
        return ESP_FAIL;
    }
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    if (sem == NULL) return ESP_FAIL;

    StorageMessage msg;
    msg.command = STORAGE_CMD_LIST_FILES;
    msg.sync_sem = sem;
    msg.payload.list_files.path = path;
    msg.payload.list_files.list = list;

    if (xQueueSend(storage_queue, &msg, pdMS_TO_TICKS(1000)) != pdPASS) {
        vSemaphoreDelete(sem);
        return ESP_FAIL;
    }

    if (xSemaphoreTake(sem, pdMS_TO_TICKS(10000)) != pdPASS) {
        vSemaphoreDelete(sem);
        return ESP_ERR_TIMEOUT;
    }

    vSemaphoreDelete(sem);
    return msg.result;
}


esp_err_t storage_read_file(const char* path, char* buffer, size_t buffer_size, size_t* bytes_read) {
    if (storage_queue == NULL) {
        ESP_LOGE(TAG, "Sistema de armazenamento não inicializado!");
        return ESP_FAIL;
    }
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    if (sem == NULL) return ESP_FAIL;

    StorageMessage msg;
    msg.command = STORAGE_CMD_READ_FILE;
    msg.sync_sem = sem;
    msg.payload.read_file.path = path;
    msg.payload.read_file.buffer = buffer;
    msg.payload.read_file.buffer_size = buffer_size;
    msg.payload.read_file.bytes_read = bytes_read;

    if (xQueueSend(storage_queue, &msg, pdMS_TO_TICKS(1000)) != pdPASS) {
        vSemaphoreDelete(sem);
        return ESP_FAIL;
    }

    if (xSemaphoreTake(sem, pdMS_TO_TICKS(10000)) != pdPASS) {
        vSemaphoreDelete(sem);
        return ESP_ERR_TIMEOUT;
    }

    vSemaphoreDelete(sem);
    return msg.result;
}

esp_err_t storage_write_file(const char* path, const void* data, size_t size, bool append) {
    if (storage_queue == NULL) {
        ESP_LOGE(TAG, "Sistema de armazenamento não inicializado!");
        return ESP_FAIL;
    }
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    if (sem == NULL) return ESP_FAIL;

    StorageMessage msg;
    msg.command = STORAGE_CMD_WRITE_FILE;
    msg.sync_sem = sem;
    msg.payload.write_file.path = path;
    msg.payload.write_file.data = data;
    msg.payload.write_file.size = size;
    msg.payload.write_file.append = append;

    // Envia a mensagem para a fila do thread de armazenamento
    if (xQueueSend(storage_queue, &msg, pdMS_TO_TICKS(1000)) != pdPASS) {
        vSemaphoreDelete(sem);
        return ESP_FAIL; // Falha ao enviar para a fila
    }

    // Espera o thread de armazenamento terminar a operação
    if (xSemaphoreTake(sem, pdMS_TO_TICKS(10000)) != pdPASS) {
        vSemaphoreDelete(sem);
        return ESP_ERR_TIMEOUT; // A operação demorou demais
    }

    vSemaphoreDelete(sem);
    return msg.result; // Retorna o resultado da operação de escrita
}