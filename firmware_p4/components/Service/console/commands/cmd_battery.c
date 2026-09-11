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

#include "console_service.h"

#include <stdio.h>
#include <string.h>

#include "esp_console.h"

#include "battery_service.h"
#include "bq25896.h"

static const char *chg_name(bq25896_charge_status_t s) {
  switch (s) {
    case CHARGE_STATUS_PRECHARGE:
      return "Pre-charge";
    case CHARGE_STATUS_FAST_CHARGE:
      return "Fast charge";
    case CHARGE_STATUS_CHARGE_DONE:
      return "Charge done";
    default:
      return "Not charging";
  }
}

static const char *vbus_name(bq25896_vbus_status_t s) {
  switch (s) {
    case VBUS_STATUS_USB_HOST:
      return "USB host";
    case VBUS_STATUS_ADAPTER_PORT:
      return "Adapter";
    case VBUS_STATUS_OTG:
      return "OTG (out)";
    default:
      return "None";
  }
}

static const char *ntc_name(uint8_t reg0c) {
  switch (reg0c & 0x07) {
    case 0x02:
      return "warm";
    case 0x03:
      return "cool";
    case 0x05:
      return "cold";
    case 0x06:
      return "hot";
    default:
      return "normal";
  }
}

static void print_summary(const bq25896_telem_t *t) {
  battery_snapshot_t bs;
  bool have_snap = battery_service_get(&bs);

  printf("Battery\n");
  printf("  Charge   : %d %%", t->soc);
  if (have_snap && bs.valid)
    printf("  (smoothed %d %%)", bs.soc);
  printf("\n");
  printf("  Voltage  : %u mV (battery)\n", t->vbat_mv);
  printf("  System   : %u mV\n", t->vsys_mv);
  printf("  State    : %s%s\n", chg_name(t->chg), t->charging ? " (charging)" : "");
  printf("  Low flag : %s\n", (have_snap && bs.low) ? "yes" : "no");

  printf("Input (VBUS)\n");
  printf("  Source   : %s\n", vbus_name(t->vbus));
  printf("  Present  : %s\n", t->power_good ? "yes" : "no");
  printf("  VBUS     : %u mV\n", t->vbus_mv);
  printf("  In limit : %u mA\n", t->iinlim_ma);
  printf("  Chg curr : %u mA\n", t->ichg_ma);

  printf("Fault\n");
  printf("  REG0C    : 0x%02X\n", t->fault);
}

static void print_diag(const bq25896_telem_t *t) {
  printf("Diagnostics (BQ25896 ADC)\n");
  printf("  VBAT     : %u mV\n", t->vbat_mv);
  printf("  VSYS     : %u mV%s\n", t->vsys_mv, t->vsys_mv ? "" : "  (ADC idle / VBAT<SYS_MIN)");
  printf("  VBUS     : %u mV%s\n", t->vbus_mv, t->power_good ? "" : "  (no VBUS)");
  printf("  ICHG     : %u mA%s\n", t->ichg_ma, t->charging ? "" : "  (not charging)");
  printf("  In limit : %u mA (effective, REG13)\n", t->iinlim_ma);
  printf("  In limit : %u mA (configured, REG00)\n", t->iinlim_set_ma);
  printf(
      "  TS       : %u.%u %%  (%s)\n", t->ts_pct_x10 / 10, t->ts_pct_x10 % 10, ntc_name(t->fault));
  printf("  Fault    : REG0C 0x%02X  chrg=%u boost=%u wd=%u bat_ovp=%u ntc=%s\n",
         t->fault,
         (t->fault >> 4) & 0x03,
         (t->fault >> 6) & 0x01,
         (t->fault >> 7) & 0x01,
         (t->fault >> 3) & 0x01,
         ntc_name(t->fault));

  static const uint8_t regs[] = {0x00, 0x0B, 0x0C, 0x0F, 0x10, 0x11, 0x12, 0x13};
  printf("Raw registers\n ");
  for (size_t i = 0; i < sizeof(regs); i++)
    printf(" 0x%02X=0x%02X", regs[i], bq25896_reg_raw(regs[i]));
  printf("\n");
}

static int cmd_battery(int argc, char **argv) {
  if (!bq25896_is_present()) {
    printf("Battery: charger (BQ25896) did not answer on I2C\n");
    return 0;
  }

  bq25896_telem_t t;
  esp_err_t err = bq25896_read_telemetry(&t);
  if (err != ESP_OK) {
    printf("Battery: telemetry read failed: %s\n", esp_err_to_name(err));
    return 1;
  }

  print_summary(&t);
  if (argc >= 2 && strcmp(argv[1], "diag") == 0)
    print_diag(&t);
  return 0;
}

void register_battery_commands(void) {
  const esp_console_cmd_t cmd_battery_def = {
      .command = "battery",
      .help = "Show battery/charger status; 'battery diag' for the full ADC dump",
      .hint = "[diag]",
      .func = &cmd_battery,
  };
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_console_cmd_register(&cmd_battery_def));
}
