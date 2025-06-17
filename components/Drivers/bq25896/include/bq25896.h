
#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c.h"


#ifdef __cplusplus
extern "C" {
#endif

// Enum de status de carregamento
typedef enum {
    BQ25896_CHG_STATUS_NOT_CHARGING = 0,
    BQ25896_CHG_STATUS_PRE_CHARGE,
    BQ25896_CHG_STATUS_FAST_CHARGE,
    BQ25896_CHG_STATUS_CHARGE_DONE
} bq25896_chg_status_t;

// Estrutura com informações completas da bateria
typedef struct {
    uint16_t battery_mV;
    uint16_t charge_current_mA;
    uint16_t vbus_mV;
    bq25896_chg_status_t status;
    bool power_good;
} bq25896_battery_info_t;

typedef struct {
    bool fault_watchdog;
    bool fault_thermal;
    bool fault_input;
    bool charge_complete;
} bq25896_fault_flags_t;

typedef struct {
    bool vbus_present;
    bool charging_enabled;
    bool otg_enabled;
} bq25896_status_flags_t;

typedef struct {
    uint16_t charge_current_mA;
    uint16_t charge_voltage_mV;
    bool otg_enabled;
} bq25896_config_t;

// === FUNÇÕES PRINCIPAIS ===

esp_err_t bq25896_attach_to_bus(void);
esp_err_t bq25896_get_battery_voltage(uint16_t *mV);
esp_err_t bq25896_get_vbus_voltage(uint16_t *mV);
esp_err_t bq25896_get_charge_current(uint16_t *mA);

esp_err_t bq25896_is_power_good(bool *pg);


// === FUNÇÕES AVANÇADAS ===

esp_err_t bq25896_get_battery_temp(int8_t *temp_c);
esp_err_t bq25896_read_all_registers(uint8_t *regs, size_t len);
esp_err_t bq25896_write_raw_register(uint8_t reg, uint8_t value);
esp_err_t bq25896_set_charge_current(uint16_t mA);
esp_err_t bq25896_set_charge_voltage(uint16_t mV);
esp_err_t bq25896_get_fault_flags(bq25896_fault_flags_t *flags);
esp_err_t bq25896_reset_chip(void);
esp_err_t bq25896_get_status_flags(bq25896_status_flags_t *flags);
esp_err_t bq25896_init_config(const bq25896_config_t *cfg);
esp_err_t bq25896_read_raw_register(uint8_t reg, uint8_t *data);

#ifdef __cplusplus
}
#endif