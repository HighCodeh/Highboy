#include "sd_card_init.h"
#include "spi.h"  // ← ADICIONE NOSSO DRIVER
#include "pin_def.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

static const char *TAG = "sd_init";

typedef enum {
    SD_INIT_CMD_INIT,
    SD_INIT_CMD_DEINIT,
    SD_INIT_CMD_REMOUNT,
    SD_INIT_CMD_CHECK_HEALTH,
    SD_INIT_CMD_IS_MOUNTED
} sd_init_command_t;

typedef struct {
    sd_init_command_t command;
    SemaphoreHandle_t sync_sem;
    
    union {
        struct {
            uint8_t max_files;
            bool format_if_failed;
            esp_err_t result;
        } init;
        
        struct {
            esp_err_t result;
        } deinit;
        
        struct {
            esp_err_t result;
        } remount;
        
        struct {
            esp_err_t result;
        } check_health;
        
        struct {
            bool result;
        } is_mounted;
    } payload;
    
    esp_err_t result;
} SDInitMessage;

static QueueHandle_t sd_init_queue = NULL;
static sdmmc_card_t *s_card = NULL;
static bool s_is_mounted = false;

static esp_err_t sd_init_internal(uint8_t max_files, bool format_if_failed) {
    if (s_is_mounted) {
        ESP_LOGW(TAG, "SD já montado");
        return ESP_OK;
    }
    
    // Adiciona SD Card como device SPI
    spi_device_config_t sd_cfg = {
        .cs_pin = SD_CARD_CS_PIN,
        .clock_speed_hz = SDMMC_FREQ_DEFAULT * 1000,  // Converte kHz para Hz
        .mode = 0,
        .queue_size = 4,
    };
    
    esp_err_t ret = spi_add_device(SPI_DEVICE_SD_CARD, &sd_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao adicionar SD no SPI: %s", esp_err_to_name(ret));
        return ret;
    }
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = format_if_failed,
        .max_files = max_files,
        .allocation_unit_size = SD_ALLOCATION_UNIT
    };
    
    ESP_LOGI(TAG, "Inicializando SD...");
    
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;
    host.slot = SPI3_HOST;
    
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CARD_CS_PIN;
    slot_config.host_id = host.slot;
    
    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, 
                                   &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao montar SD: %s", esp_err_to_name(ret));
        return ret;
    }
    
    s_is_mounted = true;
    
    // Log das informações do cartão
    sdmmc_card_print_info(stdout, s_card);
    
    ESP_LOGI(TAG, "SD montado com sucesso!");
    return ESP_OK;
}

static esp_err_t sd_deinit_internal(void) {
    if (!s_is_mounted) {
        ESP_LOGW(TAG, "SD não está montado");
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao desmontar: %s", esp_err_to_name(ret));
        return ret;
    }
    
    s_is_mounted = false;
    s_card = NULL;
    
    ESP_LOGI(TAG, "SD desmontado");
    return ESP_OK;
}

static void sd_init_thread(void *pvParameters) {
    for (;;) {
        SDInitMessage msg;
        if (xQueueReceive(sd_init_queue, &msg, portMAX_DELAY) == pdPASS) {
            
            switch (msg.command) {
                case SD_INIT_CMD_INIT: {
                    msg.payload.init.result = sd_init_internal(
                        msg.payload.init.max_files,
                        msg.payload.init.format_if_failed
                    );
                    msg.result = msg.payload.init.result;
                    break;
                }
                
                case SD_INIT_CMD_DEINIT: {
                    msg.payload.deinit.result = sd_deinit_internal();
                    msg.result = msg.payload.deinit.result;
                    break;
                }
                
                case SD_INIT_CMD_REMOUNT: {
                    if (s_is_mounted) {
                        esp_err_t ret = sd_deinit_internal();
                        if (ret != ESP_OK) {
                            msg.payload.remount.result = ret;
                            msg.result = ret;
                            break;
                        }
                    }
                    msg.payload.remount.result = sd_init_internal(SD_MAX_FILES, false);
                    msg.result = msg.payload.remount.result;
                    break;
                }
                
                case SD_INIT_CMD_CHECK_HEALTH: {
                    if (!s_is_mounted) {
                        ESP_LOGE(TAG, "SD não montado");
                        msg.payload.check_health.result = ESP_ERR_INVALID_STATE;
                    } else if (s_card == NULL) {
                        ESP_LOGE(TAG, "Ponteiro do cartão nulo");
                        msg.payload.check_health.result = ESP_FAIL;
                    } else {
                        ESP_LOGI(TAG, "Cartão saudável");
                        msg.payload.check_health.result = ESP_OK;
                    }
                    msg.result = msg.payload.check_health.result;
                    break;
                }
                
                case SD_INIT_CMD_IS_MOUNTED: {
                    msg.payload.is_mounted.result = s_is_mounted;
                    msg.result = ESP_OK;
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
esp_err_t sd_init(void) {
    if (!sd_init_queue) {
        sd_init_queue = xQueueCreate(10, sizeof(SDInitMessage));
        if (!sd_init_queue) {
            ESP_LOGE(TAG, "Falha ao criar fila");
            return ESP_FAIL;
        }
        
        if (xTaskCreate(sd_init_thread, "sd_init_thread", 4096, NULL, 5, NULL) != pdPASS) {
            vQueueDelete(sd_init_queue);
            sd_init_queue = NULL;
            ESP_LOGE(TAG, "Falha ao criar thread");
            return ESP_FAIL;
        }
    }
    
    return sd_init_custom(SD_MAX_FILES, false);
}

esp_err_t sd_init_custom(uint8_t max_files, bool format_if_failed) {
    if (!sd_init_queue) {
        return sd_init();
    }
    
    SDInitMessage msg;
    msg.command = SD_INIT_CMD_INIT;
    msg.sync_sem = xSemaphoreCreateBinary();
    if (!msg.sync_sem) {
        return ESP_FAIL;
    }
    
    msg.payload.init.max_files = max_files;
    msg.payload.init.format_if_failed = format_if_failed;
    
    xQueueSend(sd_init_queue, &msg, pdMS_TO_TICKS(1000));
    xSemaphoreTake(msg.sync_sem, pdMS_TO_TICKS(10000));
    
    esp_err_t result = msg.payload.init.result;
    vSemaphoreDelete(msg.sync_sem);
    
    return result;
}

esp_err_t sd_deinit(void) {
    if (!sd_init_queue) {
        return ESP_ERR_INVALID_STATE;
    }
    
    SDInitMessage msg;
    msg.command = SD_INIT_CMD_DEINIT;
    msg.sync_sem = xSemaphoreCreateBinary();
    if (!msg.sync_sem) {
        return ESP_FAIL;
    }
    
    xQueueSend(sd_init_queue, &msg, pdMS_TO_TICKS(1000));
    xSemaphoreTake(msg.sync_sem, pdMS_TO_TICKS(5000));
    
    esp_err_t result = msg.payload.deinit.result;
    vSemaphoreDelete(msg.sync_sem);
    
    return result;
}

bool sd_is_mounted(void) {
    if (!sd_init_queue) {
        return false;
    }
    
    SDInitMessage msg;
    msg.command = SD_INIT_CMD_IS_MOUNTED;
    msg.sync_sem = xSemaphoreCreateBinary();
    if (!msg.sync_sem) {
        return false;
    }
    
    xQueueSend(sd_init_queue, &msg, pdMS_TO_TICKS(1000));
    xSemaphoreTake(msg.sync_sem, pdMS_TO_TICKS(1000));
    
    bool result = msg.payload.is_mounted.result;
    vSemaphoreDelete(msg.sync_sem);
    
    return result;
}

esp_err_t sd_remount(void) {
    if (!sd_init_queue) {
        return ESP_ERR_INVALID_STATE;
    }
    
    SDInitMessage msg;
    msg.command = SD_INIT_CMD_REMOUNT;
    msg.sync_sem = xSemaphoreCreateBinary();
    if (!msg.sync_sem) {
        return ESP_FAIL;
    }
    
    xQueueSend(sd_init_queue, &msg, pdMS_TO_TICKS(1000));
    xSemaphoreTake(msg.sync_sem, pdMS_TO_TICKS(10000));
    
    esp_err_t result = msg.payload.remount.result;
    vSemaphoreDelete(msg.sync_sem);
    
    return result;
}

esp_err_t sd_check_health(void) {
    if (!sd_init_queue) {
        return ESP_ERR_INVALID_STATE;
    }
    
    SDInitMessage msg;
    msg.command = SD_INIT_CMD_CHECK_HEALTH;
    msg.sync_sem = xSemaphoreCreateBinary();
    if (!msg.sync_sem) {
        return ESP_FAIL;
    }
    
    xQueueSend(sd_init_queue, &msg, pdMS_TO_TICKS(1000));
    xSemaphoreTake(msg.sync_sem, pdMS_TO_TICKS(5000));
    
    esp_err_t result = msg.payload.check_health.result;
    vSemaphoreDelete(msg.sync_sem);
    
    return result;
}

sdmmc_card_t* sd_get_card_handle(void) {
    return s_card;
}