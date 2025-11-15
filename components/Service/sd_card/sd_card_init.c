/**
 * @file sd_card_init.c
 * @brief Implementação das funções de inicialização
 */
#include "sd_card_init.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

static const char *TAG = "sd_init";

static sdmmc_card_t *s_card = NULL;
static bool s_is_mounted = false;
static sdmmc_host_t s_host;
static int s_mosi = SD_PIN_MOSI;
static int s_miso = SD_PIN_MISO;
static int s_clk = SD_PIN_CLK;
static int s_cs = SD_PIN_CS;

esp_err_t sd_init(void)
{
    return sd_init_custom(SD_MAX_FILES, false);
}

esp_err_t sd_init_custom(uint8_t max_files, bool format_if_failed)
{
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

    // Inicializa host config corretamente usando compound literal
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;
    s_host = host;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = s_cs;
    slot_config.host_id = s_host.slot;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = s_mosi,
        .miso_io_num = s_miso,
        .sclk_io_num = s_clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    esp_err_t ret = spi_bus_initialize(s_host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro SPI: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &s_host, &slot_config, 
                                   &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro mount: %s", esp_err_to_name(ret));
        spi_bus_free(s_host.slot);
        return ret;
    }

    s_is_mounted = true;
    ESP_LOGI(TAG, "SD montado com sucesso!");
    return ESP_OK;
}

esp_err_t sd_init_custom_pins(int mosi, int miso, int clk, int cs)
{
    s_mosi = mosi;
    s_miso = miso;
    s_clk = clk;
    s_cs = cs;
    
    return sd_init();
}

esp_err_t sd_deinit(void)
{
    if (!s_is_mounted) {
        ESP_LOGW(TAG, "SD não está montado");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro unmount: %s", esp_err_to_name(ret));
        return ret;
    }

    spi_bus_free(s_host.slot);
    s_is_mounted = false;
    s_card = NULL;
    
    ESP_LOGI(TAG, "SD desmontado");
    return ESP_OK;
}

bool sd_is_mounted(void)
{
    return s_is_mounted;
}

esp_err_t sd_remount(void)
{
    if (s_is_mounted) {
        esp_err_t ret = sd_deinit();
        if (ret != ESP_OK) return ret;
    }
    return sd_init();
}

esp_err_t sd_reset_bus(void)
{
    if (s_is_mounted) {
        sd_deinit();
    }
    
    spi_bus_free(s_host.slot);
    ESP_LOGI(TAG, "Barramento SPI resetado");
    
    return sd_init();
}

esp_err_t sd_check_health(void)
{
    if (!s_is_mounted) {
        ESP_LOGE(TAG, "SD não montado");
        return ESP_ERR_INVALID_STATE;
    }

    if (s_card == NULL) {
        ESP_LOGE(TAG, "Ponteiro do cartão nulo");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Cartão saudável");
    return ESP_OK;
}

sdmmc_card_t* sd_get_card_handle(void)
{
    return s_card;
}