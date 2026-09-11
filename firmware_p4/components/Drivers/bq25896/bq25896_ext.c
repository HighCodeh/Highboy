// Copyright (c) 2025 HIGH CODE LLC
//
// TentacleOS is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// TentacleOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with TentacleOS. If not, see <https://www.gnu.org/licenses/>.

#include "bq25896.h"

#define BQ_SYSV_BASE_MV      2304
#define BQ_SYSV_STEP_MV      20
#define BQ_SYSV_MASK         0x7F
#define BQ_VBUS_BASE_MV      2600
#define BQ_VBUS_STEP_MV      100
#define BQ_VBUS_MASK         0x7F
#define BQ_VBUS_GD_MASK      0x80
#define BQ_ICHG_STEP_MA      50
#define BQ_ICHG_MASK         0x7F
#define BQ_IDPM_BASE_MA      100
#define BQ_IDPM_STEP_MA      50
#define BQ_IDPM_MASK         0x3F
#define BQ_ILIM_BASE_MA      100
#define BQ_ILIM_STEP_MA      50
#define BQ_ILIM_MASK         0x3F
#define BQ_TS_MASK           0x7F
#define BQ_TS_BASE_PCT_X1000 21000
#define BQ_TS_STEP_PCT_X1000 465

#define BQ_REG_ILIM     0x00
#define BQ_REG_SYS_VOLT 0x0F
#define BQ_REG_TS_ADC   0x10
#define BQ_REG_VBUS_ADC 0x11
#define BQ_REG_ICHG_ADC 0x12
#define BQ_REG_IDPM_ADC 0x13

esp_err_t bq25896_read_telemetry(bq25896_telem_t *out) {
  if (out == NULL)
    return ESP_ERR_INVALID_ARG;

  uint16_t vbat = bq25896_get_battery_voltage();
  out->vbat_mv = vbat;
  out->soc = bq25896_get_battery_percentage(vbat);
  out->chg = bq25896_get_charge_status();
  out->vbus = bq25896_get_vbus_status();
  out->charging = bq25896_is_charging();

  uint8_t sysv = bq25896_reg_raw(BQ_REG_SYS_VOLT);
  out->vsys_mv = sysv ? (uint16_t)(BQ_SYSV_BASE_MV + (sysv & BQ_SYSV_MASK) * BQ_SYSV_STEP_MV) : 0;

  uint8_t vbusr = bq25896_reg_raw(BQ_REG_VBUS_ADC);
  bool vbus_gd = (vbusr & BQ_VBUS_GD_MASK) != 0;
  out->vbus_mv =
      vbus_gd ? (uint16_t)(BQ_VBUS_BASE_MV + (vbusr & BQ_VBUS_MASK) * BQ_VBUS_STEP_MV) : 0;
  out->power_good = vbus_gd;

  out->ichg_ma = (uint16_t)((bq25896_reg_raw(BQ_REG_ICHG_ADC) & BQ_ICHG_MASK) * BQ_ICHG_STEP_MA);
  out->iinlim_ma = (uint16_t)(BQ_IDPM_BASE_MA +
                              (bq25896_reg_raw(BQ_REG_IDPM_ADC) & BQ_IDPM_MASK) * BQ_IDPM_STEP_MA);
  out->iinlim_set_ma =
      (uint16_t)(BQ_ILIM_BASE_MA + (bq25896_reg_raw(BQ_REG_ILIM) & BQ_ILIM_MASK) * BQ_ILIM_STEP_MA);

  uint8_t tsr = bq25896_reg_raw(BQ_REG_TS_ADC) & BQ_TS_MASK;
  out->ts_pct_x10 = (uint16_t)((BQ_TS_BASE_PCT_X1000 + tsr * BQ_TS_STEP_PCT_X1000) / 100);

  out->fault = bq25896_get_fault();
  return ESP_OK;
}
