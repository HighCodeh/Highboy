// bq25896.h
#ifndef BQ25896_H
#define BQ25896_H

#include "driver/i2c.h"
#include <stdbool.h>

// Endereço I2C do BQ25896
#define BQ25896_I2C_ADDR 0x6B

// Enum para o Status de Carregamento (conforme datasheet)
typedef enum {
    CHARGE_STATUS_NOT_CHARGING = 0,
    CHARGE_STATUS_PRECHARGE = 1,
    CHARGE_STATUS_FAST_CHARGE = 2,
    CHARGE_STATUS_CHARGE_DONE = 3
} bq25896_charge_status_t;

// Enum para o Status do VBUS (conforme datasheet)
typedef enum {
    VBUS_STATUS_UNKNOWN = 0,
    VBUS_STATUS_USB_HOST = 1,
    VBUS_STATUS_ADAPTER_PORT = 2,
    VBUS_STATUS_OTG = 3
} bq25896_vbus_status_t;


// --- Declaração das Funções Públicas ---

// Inicializa o BQ25896
esp_err_t bq25896_init(void);

// Obtém o status de carregamento
bq25896_charge_status_t bq25896_get_charge_status(void);

// Obtém a tensão da bateria em mV
uint16_t bq25896_get_battery_voltage(void);

// Obtém o status do VBUS (se o carregador está conectado)
bq25896_vbus_status_t bq25896_get_vbus_status(void);

// Retorna 'true' se estiver carregando (pré-carga ou carga rápida)
bool bq25896_is_charging(void);

// Estima a porcentagem da bateria com base na tensão
int bq25896_get_battery_percentage(uint16_t voltage_mv);

#endif // BQ25896_H