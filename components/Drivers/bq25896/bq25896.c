#include "bq25896.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h" // Inclua esp_err.h para ESP_OK e ESP_FAIL
#include "freertos/FreeRTOS.h" // Para pdMS_TO_TICKS
#include "freertos/task.h"    // Para vTaskDelay

#define I2C_PORT I2C_NUM_0
#define BQ25896_I2C_ADDR 0x6B// Endereço I2C do BQ25896. VERIFIQUE O SEU DATASHEET!

static const char *TAG = "BQ25896";

// A flag bq_initialized deve ser usada para controlar o estado geral do driver,
// não para impedir a própria função de inicialização.
static bool bq_driver_ready = false; // Nova flag para indicar se o driver está pronto após attach_to_bus

// Função interna para ler um registrador I2C
// Esta função agora assume que i2c_master_init() já foi chamado.
static esp_err_t bq25896_read_reg_internal(uint8_t reg, uint8_t *data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BQ25896_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BQ25896_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        // Não logamos um erro aqui se bq_driver_ready for falso,
        // pois a função de anexo fará o log.
        if (bq_driver_ready) { // Só loga se já deveria estar pronto e falhou
             ESP_LOGE(TAG, "Falha ao ler registrador 0x%02X: %s", reg, esp_err_to_name(ret));
        }
    }
    return ret;
}

// Função interna para escrever em um registrador I2C
static esp_err_t bq25896_write_reg_internal(uint8_t reg, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BQ25896_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        if (bq_driver_ready) { // Só loga se já deveria estar pronto e falhou
             ESP_LOGE(TAG, "Falha ao escrever no registrador 0x%02X com dado 0x%02X: %s", reg, data, esp_err_to_name(ret));
        }
    }
    return ret;
}

// Anexa o BQ25896 ao barramento I2C e verifica sua presença
esp_err_t bq25896_attach_to_bus(void) {
    // Tenta ler um registrador conhecido que geralmente tem um valor padrão,
    // como o registrador de ID do dispositivo (se existir) ou um registrador de status.
    // O registrador 0x0B (SYS_STAT) é um bom candidato para verificar comunicação.
    uint8_t reg_val;
    // Usamos a função interna de leitura que não verifica `bq_driver_ready` para a inicialização.
    if (bq25896_read_reg_internal(BQ25896_REG_STATUS, &reg_val) == ESP_OK) {
        bq_driver_ready = true; // Definimos a flag como true APENAS se a comunicação inicial for bem-sucedida
        ESP_LOGI(TAG, "BQ25896 detectado e operacional. Status inicial: 0x%02X", reg_val);
        return ESP_OK;
    }
    bq_driver_ready = false; // Garante que a flag é falsa em caso de falha na inicialização
    ESP_LOGE(TAG, "Falha ao detectar BQ25896. Verifique as conexões I2C e o endereço (0x%02X).", BQ25896_I2C_ADDR);
    return ESP_FAIL;
}

// Funções públicas que usarão a flag bq_driver_ready
uint16_t bq25896_get_battery_voltage(void) {
    if (!bq_driver_ready) {
        ESP_LOGE(TAG, "BQ25896 não está pronto. Chame bq25896_attach_to_bus() primeiro.");
        return 0;
    }
    uint8_t bat_voltage_reg_val;
    if (bq25896_read_reg_internal(BQ25896_REG_BAT_VOLTAGE, &bat_voltage_reg_val) == ESP_OK) {
        // VERIFIQUE O DATASHEET DO BQ25896 PARA O CÁLCULO CORRETO!
        return ((uint16_t)bat_voltage_reg_val * 20) + 2304; // Em mV
    }
    return 0; // Retorna 0 em caso de erro
}

bq25896_charge_status_t bq25896_get_charge_status(void) {
    if (!bq_driver_ready) {
        ESP_LOGE(TAG, "BQ25896 não está pronto. Chame bq25896_attach_to_bus() primeiro.");
        return CHARGE_STATUS_UNKNOWN;
    }
    uint8_t status_reg_val;
    if (bq25896_read_reg_internal(BQ25896_REG_STATUS, &status_reg_val) == ESP_OK) {
        uint8_t charge_status = (status_reg_val & BQ25896_CHRG_STAT_MASK) >> BQ25896_CHRG_STAT_SHIFT;
        switch (charge_status) {
            case 0b00: return CHARGE_STATUS_NOT_CHARGING;
            case 0b01: return CHARGE_STATUS_PRECHARGE;
            case 0b10: return CHARGE_STATUS_FAST_CHARGE;
            case 0b11: return CHARGE_STATUS_CHARGE_DONE;
            default: return CHARGE_STATUS_UNKNOWN;
        }
    }
    return CHARGE_STATUS_UNKNOWN; // Retorna status desconhecido em caso de erro
}

bool bq25896_is_charging(void) {
    if (!bq_driver_ready) {
        return false; // Não está carregando se o driver não estiver pronto
    }
    bq25896_charge_status_t status = bq25896_get_charge_status();
    return (status == CHARGE_STATUS_PRECHARGE || status == CHARGE_STATUS_FAST_CHARGE);
}

// Calcula a porcentagem da bateria com base na voltagem
int bq25896_get_battery_percentage(uint16_t voltage_mv) {
    // Esta função não precisa de bq_driver_ready, pois opera nos dados já obtidos.
    const uint16_t VOLTAGE_MAX = 4200; // 4.2V (100% carga)
    const uint16_t VOLTAGE_MIN = 3000; // 3.0V (0% carga, ponto de corte)

    if (voltage_mv >= VOLTAGE_MAX) {
        return 100;
    } else if (voltage_mv <= VOLTAGE_MIN) {
        return 0;
    } else {
        int percentage = (int)(((float)(voltage_mv - VOLTAGE_MIN) / (VOLTAGE_MAX - VOLTAGE_MIN)) * 100.0f);
        return percentage;
    }
}