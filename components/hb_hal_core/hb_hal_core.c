#include "hb_hal_core.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_log.h"
#include "esp_err.h"

// Tag para o sistema de log do ESP-IDF
static const char *TAG = "HB_HAL_CORE";

/**
 * @brief Inicializa os recursos básicos do sistema.
 *
 * Executa as seguintes etapas:
 * - Inicialização do sistema de log.
 * - Leitura e exibição das características do chip ESP32‑S3.
 * - Registro e exibição do motivo do último reset.
 * - Execução de outras configurações essenciais (ex.: inicialização de clocks,
 *   configurações de memória, etc.) que podem ser adicionadas conforme a necessidade.
 *
 * @return hb_hal_status_t HB_HAL_OK se tudo ocorrer corretamente;
 *         caso contrário, um código de erro adequado.
 */
hb_hal_status_t hb_hal_core_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Iniciando a inicialização do núcleo do sistema para ESP32-S3");

    // Obter informações do chip
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "Chip: ESP32-S3, %d core(s), Rev. %d", chip_info.cores, chip_info.revision);

    // Exibir o motivo do último reset
    const char *reset_reason = hb_hal_core_get_reset_reason();
    ESP_LOGI(TAG, "Motivo do último reset: %s", reset_reason);

    // Aqui, podem ser incluídas outras inicializações críticas, como:
    // - Configuração de clocks e PLLs
    // - Inicialização de periféricos essenciais (ex.: UART para debug)
    // - Configurações de memória e watchdog timers
    // Caso alguma etapa falhe, retorne HB_HAL_INIT_FAIL ou outro código adequado.

    ESP_LOGI(TAG, "Inicialização do núcleo concluída com sucesso");
    return HB_HAL_OK;
}

/**
 * @brief Reinicializa o sistema.
 *
 * Em ambiente de produção, esta função chama esp_restart(), que efetua o reset do chip.
 * Note que esta função, se bem-sucedida, não retorna.
 *
 * @return hb_hal_status_t HB_HAL_OK (apenas para conformidade, pois o reset não retorna).
 */
hb_hal_status_t hb_hal_core_reboot(void)
{
    ESP_LOGI(TAG, "Reinicialização do sistema iniciada...");
    // Em um ambiente real, os logs podem ser finalizados ou enviados antes do reset
    esp_restart();
    return HB_HAL_OK;
}

/**
 * @brief Mapeia o motivo do reset para uma mensagem descritiva.
 *
 * Utiliza a função esp_reset_reason() do ESP-IDF para determinar o
 * motivo do reset e retorna uma string correspondente.
 *
 * @return const char* String com a descrição do reset.
 */
const char* hb_hal_core_get_reset_reason(void)
{
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason)
    {
        case ESP_RST_POWERON:
            return "Power on reset";
        case ESP_RST_EXT:
            return "External reset";
        case ESP_RST_SW:
            return "Software reset";
        case ESP_RST_PANIC:
            return "Panic reset";
        case ESP_RST_INT_WDT:
            return "Interrupt watchdog reset";
        case ESP_RST_TASK_WDT:
            return "Task watchdog reset";
        case ESP_RST_WDT:
            return "Other watchdog reset";
        case ESP_RST_DEEPSLEEP:
            return "Deep sleep reset";
        case ESP_RST_BROWNOUT:
            return "Brownout reset";
        case ESP_RST_SDIO:
            return "SDIO reset";
        default:
            return "Unknown reset reason";
    }
}
