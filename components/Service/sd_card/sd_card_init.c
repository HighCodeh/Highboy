#include "sd_card_init.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
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
    SD_INIT_CMD_RESET_BUS,
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
            int mosi;
            int miso;
            int clk;
            int cs;
            bool use_custom_pins;
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
        } reset_bus;
        
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
static sdmmc_host_t s_host;
static spi_host_device_t s_spi_host = SPI3_HOST;

// Pinos padrão
static int s_mosi = SD_PIN_MOSI;
static int s_miso = SD_PIN_MISO;
static int s_clk = SD_PIN_CLK;
static int s_cs = SD_PIN_CS;

static esp_err_t sd_init_internal(uint8_t max_files, bool format_if_failed) {
    if (s_is_mounted) {
        ESP_LOGW(TAG, "SD já montado");
        return ESP_OK;
    }
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = format_if_failed,
        .max_files = max_files,
        .allocation_unit_size = SD_ALLOCATION_UNIT
    };
    
    ESP_LOGI(TAG, "Inicializando SD...");
    
    // Configura host
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;
    host.slot = s_spi_host;
    s_host = host;
    
    // Configura slot
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = s_cs;
    slot_config.host_id = s_host.slot;
    
    // Configura barramento SPI
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = s_mosi,
        .miso_io_num = s_miso,
        .sclk_io_num = s_clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    
    esp_err_t ret = spi_bus_initialize(s_host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Erro ao inicializar SPI: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &s_host, &slot_config, 
                                   &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao montar SD: %s", esp_err_to_name(ret));
        spi_bus_free(s_host.slot);
        return ret;
    }
    
    s_is_mounted = true;
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
    
    spi_bus_free(s_host.slot);
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
                    // Atualiza pinos
                    if (msg.payload.init.use_custom_pins) {
                        s_mosi = msg.payload.init.mosi;
                        s_miso = msg.payload.init.miso;
                        s_clk = msg.payload.init.clk;
                        s_cs = msg.payload.init.cs;
                    }
                    
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
                
                case SD_INIT_CMD_RESET_BUS: {
                    if (s_is_mounted) {
                        sd_deinit_internal();
                    }
                    spi_bus_free(s_host.slot);
                    ESP_LOGI(TAG, "Barramento SPI resetado");
                    
                    msg.payload.reset_bus.result = sd_init_internal(SD_MAX_FILES, false);
                    msg.result = msg.payload.reset_bus.result;
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
            
            // Libera semáforo para sincronização
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
        return sd_init(); // Inicializa a thread
    }
    
    SDInitMessage msg;
    msg.command = SD_INIT_CMD_INIT;
    msg.sync_sem = xSemaphoreCreateBinary();
    if (!msg.sync_sem) {
        return ESP_FAIL;
    }
    
    msg.payload.init.max_files = max_files;
    msg.payload.init.format_if_failed = format_if_failed;
    msg.payload.init.use_custom_pins = false;
    
    xQueueSend(sd_init_queue, &msg, pdMS_TO_TICKS(1000));
    xSemaphoreTake(msg.sync_sem, pdMS_TO_TICKS(10000));
    
    esp_err_t result = msg.payload.init.result;
    vSemaphoreDelete(msg.sync_sem);
    
    return result;
}

esp_err_t sd_init_custom_pins(int mosi, int miso, int clk, int cs) {
    if (!sd_init_queue) {
        esp_err_t ret = sd_init(); // Inicializa a thread
        if (ret != ESP_OK) return ret;
    }
    
    SDInitMessage msg;
    msg.command = SD_INIT_CMD_INIT;
    msg.sync_sem = xSemaphoreCreateBinary();
    if (!msg.sync_sem) {
        return ESP_FAIL;
    }
    
    msg.payload.init.max_files = SD_MAX_FILES;
    msg.payload.init.format_if_failed = false;
    msg.payload.init.use_custom_pins = true;
    msg.payload.init.mosi = mosi;
    msg.payload.init.miso = miso;
    msg.payload.init.clk = clk;
    msg.payload.init.cs = cs;
    
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

esp_err_t sd_reset_bus(void) {
    if (!sd_init_queue) {
        return ESP_ERR_INVALID_STATE;
    }
    
    SDInitMessage msg;
    msg.command = SD_INIT_CMD_RESET_BUS;
    msg.sync_sem = xSemaphoreCreateBinary();
    if (!msg.sync_sem) {
        return ESP_FAIL;
    }
    
    xQueueSend(sd_init_queue, &msg, pdMS_TO_TICKS(1000));
    xSemaphoreTake(msg.sync_sem, pdMS_TO_TICKS(10000));
    
    esp_err_t result = msg.payload.reset_bus.result;
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