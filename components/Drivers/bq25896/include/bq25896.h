#ifndef BQ25896_H
#define BQ25896_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

// Endereços dos registradores do BQ25896
// Você precisará consultar o datasheet do BQ25896 para obter os endereços corretos dos registradores.
// Estes são exemplos e podem precisar ser ajustados.
#define BQ25896_REG_STATUS      0x0B // Register 0Bh: System Status Register (SYS_STAT)
#define BQ25896_REG_VBUS_STAT   0x0C // Register 0Ch: VBUS Status Register (VBUS_STAT)
#define BQ25896_REG_BAT_VOLTAGE 0x0E // Register 0Eh: Battery Voltage Register (BAT_V) - Exemplo, consulte o datasheet

// Definição para os bits de status de carga no registrador SYS_STAT (0x0B)
// Consulte o datasheet para os bits exatos.
#define BQ25896_CHRG_STAT_MASK      0x18 // Bits 4 e 3 (CHRG_STAT[1:0])
#define BQ25896_CHRG_STAT_SHIFT     3

// Enumeração para o status de carga
typedef enum {
    CHARGE_STATUS_NOT_CHARGING = 0, // Not Charging
    CHARGE_STATUS_PRECHARGE = 1,    // Precharge
    CHARGE_STATUS_FAST_CHARGE = 2,  // Fast Charging
    CHARGE_STATUS_CHARGE_DONE = 3,  // Charge Done
    CHARGE_STATUS_UNKNOWN = 4       // Status desconhecido ou erro de leitura
} bq25896_charge_status_t;


esp_err_t bq25896_attach_to_bus(void);
uint16_t bq25896_get_battery_voltage(void);
int bq25896_get_battery_percentage(uint16_t voltage_mv);
bq25896_charge_status_t bq25896_get_charge_status(void);
bool bq25896_is_charging(void);

#endif // BQ25896_H