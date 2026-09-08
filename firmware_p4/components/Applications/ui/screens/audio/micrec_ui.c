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

#include "micrec_ui.h"

#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "audio_i2s.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sys_prio.h"

#include "menu_component_ui.h"
#include "notify_ui.h"
#include "ui_chrome.h"
#include "ui_manager.h"
#include "ui_theme.h"
#include "vfs_core.h"

static const char *TAG = "MICREC_UI";

#define STATUS_TICK_MS     50
#define REC_RATE           16000
#define REC_MAX_SECONDS    600
#define REC_CHUNK          512
#define REC_WARMUP         4
#define REC_MAX_SAMPLES    ((uint32_t)REC_RATE * REC_MAX_SECONDS)
#define REC_MIN_FREE       (256 * 1024)
#define ACCENT_GREEN       0x00E676
#define ACCENT_RED         0xFF3B30
#define VU_FULLSCALE_PEAK  7000
#define SCOPE_W            112
#define SCOPE_FULL         9000.0f
#define OV_BOX_W           210
#define OV_BOX_H           132
#define WF_X0              12
#define WF_X1              198
#define WF_CY              60
#define WF_AMP             22
#define OVERLAY_TICK_MS    60
#define OVERLAY_HIDE_MS    700
#define MIC_TASK_STACK     8192
#define MIC_TASK_PRIORITY  SYS_PRIO_SERVICE_LO

#define REC_DIR       "/sdcard/recordings"
#define WAV_HDR_LEN   44
#define WAV_FMT_CHUNK 16
#define WAV_FMT_PCM   1
#define WAV_CH_MONO   1
#define WAV_BITS      16
#define SAVE_MAX_IDX  999

#define CLIP_THRESH       32000
#define VU_HOLD_DECAY_PCT 2
#define BAR_MARGIN        14
#define BAR_W             (OV_BOX_W - 2 * BAR_MARGIN)
#define BAR_Y0            (OV_BOX_H - 21)
#define HOLD_W            3

#define REC_TICK_MS 50
#define REC_TOP     (UI_CHROME_HEADER_H + 8)
#define REC_WF_X0   10
#define REC_WF_X1   230
#define REC_WF_CY   (UI_CHROME_HEADER_H + 96)
#define REC_WF_AMP  50
#define REC_BAR_X0  16
#define REC_BAR_W   208
#define REC_DB_Y    (UI_CHROME_HEADER_H + 158)
#define REC_BAR_Y   (UI_CHROME_HEADER_H + 180)

enum { ROW_LEVEL, ROW_REC, ROW_PLAY, ROW_LOOP, ROW_COUNT };
enum { OV_TIME, OV_VU };
enum { ST_IDLE, ST_RECORDING, ST_PLAYING };

static lv_obj_t *s_screen = NULL;
static menu_component_t s_menu;
static lv_timer_t *s_status_timer = NULL;
static lv_obj_t *s_status_lbl = NULL;

static lv_obj_t *s_ov = NULL;
static lv_obj_t *s_ov_dot = NULL;
static lv_obj_t *s_ov_state = NULL;
static lv_obj_t *s_ov_time = NULL;
static lv_obj_t *s_ov_db = NULL;
static lv_obj_t *s_ov_bar = NULL;
static lv_obj_t *s_ov_hold = NULL;
static lv_obj_t *s_scope_line = NULL;
static lv_obj_t *s_scope_line2 = NULL;
static lv_timer_t *s_ov_timer = NULL;
static int s_ov_mode = OV_TIME;
static uint32_t s_ov_start = 0;
static uint32_t s_ov_total = 1;
static volatile bool s_is_op_done = false;
static char s_done_text[24] = "";
static volatile int s_live_peak = 0;
static volatile int s_live_rms = 0;
static volatile bool s_is_clipped = false;
static int s_vu_display = 0;
static int s_vu_hold = 0;

static volatile int16_t s_scope[SCOPE_W];
static volatile int s_scope_head = 0;
static lv_point_precise_t s_scope_pts[SCOPE_W];
static lv_point_precise_t s_scope_pts2[SCOPE_W];

static volatile bool s_is_busy = false;
static volatile bool s_is_stop_req = false;
static volatile int s_state = ST_IDLE;
static bool s_is_loop = false;
static uint32_t s_last_ms = 0;
static int s_rec_gain_q8 = 256;
static char s_last_path[80] = "";
static volatile bool s_is_paused = false;
static volatile uint32_t s_rec_total = 0;
static volatile bool s_is_rec_ok = false;

static lv_obj_t *s_rec_scr = NULL;
static lv_obj_t *s_rec_dot = NULL;
static lv_obj_t *s_rec_state = NULL;
static lv_obj_t *s_rec_time = NULL;
static lv_obj_t *s_rec_db = NULL;
static lv_obj_t *s_rec_bar = NULL;
static lv_obj_t *s_rec_hold = NULL;
static lv_obj_t *s_rec_wf = NULL;
static lv_obj_t *s_rec_wf2 = NULL;
static lv_timer_t *s_rec_timer = NULL;
static lv_point_precise_t s_rec_pts[SCOPE_W];
static lv_point_precise_t s_rec_pts2[SCOPE_W];

static void rec_screen_build(void);
static void rec_finish(const char *msg);

static void scope_reset(void) {
  for (int i = 0; i < SCOPE_W; i++)
    s_scope[i] = 0;
  s_scope_head = 0;
}

static void scope_push(int peak) {
  int h = s_scope_head;
  s_scope[h] = (int16_t)(peak > 32767 ? 32767 : peak);
  s_scope_head = (h + 1) % SCOPE_W;
}

static void scope_redraw(void) {
  if (s_scope_line == NULL)
    return;
  int head = s_scope_head;
  for (int j = 0; j < SCOPE_W; j++) {
    int idx = (head + j) % SCOPE_W;
    float v = (float)s_scope[idx] / SCOPE_FULL;
    if (v > 1.0f)
      v = 1.0f;
    int x = WF_X0 + j * (WF_X1 - WF_X0) / (SCOPE_W - 1);
    int dy = (int)(v * (float)WF_AMP);
    s_scope_pts[j].x = x;
    s_scope_pts[j].y = WF_CY - dy;
    s_scope_pts2[j].x = x;
    s_scope_pts2[j].y = WF_CY + dy;
  }
  lv_obj_invalidate(s_scope_line);
  if (s_scope_line2)
    lv_obj_invalidate(s_scope_line2);
}

static void fmt_mmss(char *buf, size_t sz, uint32_t ms) {
  uint32_t s = ms / 1000;
  snprintf(buf, sz, "%u:%02u", (unsigned)(s / 60), (unsigned)(s % 60));
}

static void overlay_hide_cb(lv_timer_t *t) {
  lv_timer_delete(t);
  if (s_ov) {
    lv_obj_del(s_ov);
    s_ov = NULL;
    s_ov_dot = NULL;
    s_ov_state = NULL;
    s_ov_time = NULL;
    s_ov_db = NULL;
    s_ov_bar = NULL;
    s_ov_hold = NULL;
    s_scope_line = NULL;
    s_scope_line2 = NULL;
  }
}

static void overlay_tick(lv_timer_t *t) {
  if (s_ov_bar == NULL) {
    lv_timer_delete(t);
    s_ov_timer = NULL;
    return;
  }
  if (s_is_op_done) {
    lv_bar_set_value(s_ov_bar, 100, LV_ANIM_OFF);
    if (s_ov_state)
      lv_label_set_text(s_ov_state, s_done_text);
    if (s_ov_dot)
      lv_obj_set_style_bg_opa(s_ov_dot, LV_OPA_COVER, 0);
    if (s_scope_line)
      lv_obj_add_flag(s_scope_line, LV_OBJ_FLAG_HIDDEN);
    if (s_scope_line2)
      lv_obj_add_flag(s_scope_line2, LV_OBJ_FLAG_HIDDEN);
    if (s_ov_hold)
      lv_obj_add_flag(s_ov_hold, LV_OBJ_FLAG_HIDDEN);
    if (s_ov_db)
      lv_obj_add_flag(s_ov_db, LV_OBJ_FLAG_HIDDEN);
    lv_timer_delete(t);
    s_ov_timer = NULL;
    lv_timer_t *h = lv_timer_create(overlay_hide_cb, OVERLAY_HIDE_MS, NULL);
    lv_timer_set_repeat_count(h, 1);
    return;
  }
  uint32_t elapsed = lv_tick_get() - s_ov_start;
  char tbuf[12];
  fmt_mmss(tbuf, sizeof(tbuf), elapsed);
  if (s_ov_time)
    lv_label_set_text(s_ov_time, tbuf);

  if (s_ov_mode == OV_VU) {
    if (s_ov_dot) {
      uint32_t ph = elapsed % 1000;
      uint32_t tri = ph < 500 ? ph : 1000 - ph;
      lv_obj_set_style_bg_opa(s_ov_dot, (lv_opa_t)(90 + tri * 165 / 500), 0);
    }
    int pct = (int)((int64_t)s_live_peak * 100 / VU_FULLSCALE_PEAK);
    if (pct > 100)
      pct = 100;
    if (pct > s_vu_display)
      s_vu_display = pct;
    else
      s_vu_display = (s_vu_display * 7) / 10;
    lv_bar_set_value(s_ov_bar, s_vu_display, LV_ANIM_OFF);
    if (pct > s_vu_hold)
      s_vu_hold = pct;
    else if (s_vu_hold > 0)
      s_vu_hold -= VU_HOLD_DECAY_PCT;
    if (s_vu_hold < 0)
      s_vu_hold = 0;
    if (s_ov_hold)
      lv_obj_set_pos(s_ov_hold, BAR_MARGIN + s_vu_hold * (BAR_W - HOLD_W) / 100, BAR_Y0);
    scope_redraw();
    if (s_ov_db) {
      if (s_is_clipped) {
        lv_obj_set_style_text_color(s_ov_db, lv_color_hex(ACCENT_RED), 0);
        lv_label_set_text(s_ov_db, "CLIP");
      } else {
        int rms = s_live_rms < 1 ? 1 : s_live_rms;
        int db = (int)(20.0f * log10f((float)rms / 32768.0f));
        char dbuf[16];
        snprintf(dbuf, sizeof(dbuf), "%d dBFS", db);
        lv_label_set_text(s_ov_db, dbuf);
      }
    }
  } else {
    int pct = (int)((uint64_t)elapsed * 100 / s_ov_total);
    if (pct > 99)
      pct = 99;
    lv_bar_set_value(s_ov_bar, pct, LV_ANIM_OFF);
  }
}

static void overlay_show(const char *title, uint32_t total_ms, int mode) {
  uint32_t accent = (mode == OV_VU) ? ACCENT_RED : ACCENT_GREEN;
  if (s_ov == NULL) {
    s_ov = lv_obj_create(s_screen);
    lv_obj_set_size(s_ov, LV_PCT(100), LV_PCT(100));
    lv_obj_center(s_ov);
    lv_obj_remove_flag(s_ov, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_ov, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_ov, LV_OPA_80, 0);
    lv_obj_set_style_border_width(s_ov, 0, 0);
    lv_obj_set_style_pad_all(s_ov, 0, 0);

    lv_obj_t *box = lv_obj_create(s_ov);
    lv_obj_set_size(box, OV_BOX_W, OV_BOX_H);
    lv_obj_center(box);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(box, current_theme.bg_secondary, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(box, current_theme.border_inactive, 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_radius(box, 14, 0);
    lv_obj_set_style_pad_all(box, 0, 0);

    s_ov_dot = lv_obj_create(box);
    lv_obj_set_size(s_ov_dot, 11, 11);
    lv_obj_set_pos(s_ov_dot, 14, 13);
    lv_obj_remove_flag(s_ov_dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(s_ov_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_ov_dot, 0, 0);
    lv_obj_set_style_pad_all(s_ov_dot, 0, 0);

    s_ov_state = lv_label_create(box);
    lv_obj_set_style_text_font(s_ov_state, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(s_ov_state, 32, 13);

    s_ov_time = lv_label_create(box);
    lv_obj_set_style_text_font(s_ov_time, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_ov_time, current_theme.text_main, 0);
    lv_obj_align(s_ov_time, LV_ALIGN_TOP_RIGHT, -14, 9);

    s_scope_line = lv_line_create(box);
    lv_obj_set_style_line_width(s_scope_line, 2, 0);
    lv_obj_set_pos(s_scope_line, 0, 0);
    s_scope_line2 = lv_line_create(box);
    lv_obj_set_style_line_width(s_scope_line2, 2, 0);
    lv_obj_set_pos(s_scope_line2, 0, 0);
    for (int j = 0; j < SCOPE_W; j++) {
      int x = WF_X0 + j * (WF_X1 - WF_X0) / (SCOPE_W - 1);
      s_scope_pts[j].x = x;
      s_scope_pts[j].y = WF_CY;
      s_scope_pts2[j].x = x;
      s_scope_pts2[j].y = WF_CY;
    }
    lv_line_set_points_mutable(s_scope_line, s_scope_pts, SCOPE_W);
    lv_line_set_points_mutable(s_scope_line2, s_scope_pts2, SCOPE_W);

    s_ov_db = lv_label_create(box);
    lv_obj_set_style_text_font(s_ov_db, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_ov_db, current_theme.text_main, 0);
    lv_obj_set_style_text_opa(s_ov_db, LV_OPA_70, 0);
    lv_obj_align(s_ov_db, LV_ALIGN_BOTTOM_LEFT, 14, -26);

    s_ov_bar = lv_bar_create(box);
    lv_obj_set_size(s_ov_bar, OV_BOX_W - 28, 9);
    lv_obj_align(s_ov_bar, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_bar_set_range(s_ov_bar, 0, 100);
    lv_obj_set_style_bg_color(s_ov_bar, lv_color_hex(0x202028), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_ov_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(s_ov_bar, 5, LV_PART_MAIN);
    lv_obj_set_style_radius(s_ov_bar, 5, LV_PART_INDICATOR);

    s_ov_hold = lv_obj_create(box);
    lv_obj_set_size(s_ov_hold, HOLD_W, 9);
    lv_obj_set_pos(s_ov_hold, BAR_MARGIN, BAR_Y0);
    lv_obj_remove_flag(s_ov_hold, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(s_ov_hold, 0, 0);
    lv_obj_set_style_pad_all(s_ov_hold, 0, 0);
    lv_obj_set_style_radius(s_ov_hold, 1, 0);
    lv_obj_set_style_bg_color(s_ov_hold, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(s_ov_hold, LV_OPA_COVER, 0);
  }

  lv_obj_set_style_bg_color(s_ov_dot, lv_color_hex(accent), 0);
  lv_obj_set_style_bg_opa(s_ov_dot, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(s_ov_state, lv_color_hex(accent), 0);
  lv_label_set_text(s_ov_state, title);
  lv_obj_set_style_line_color(s_scope_line, lv_color_hex(accent), 0);
  lv_obj_set_style_line_color(s_scope_line2, lv_color_hex(accent), 0);
  lv_obj_set_style_bg_color(s_ov_bar, lv_color_hex(accent), LV_PART_INDICATOR);
  lv_bar_set_value(s_ov_bar, 0, LV_ANIM_OFF);
  lv_label_set_text(s_ov_time, "0:00");

  bool wave = (mode == OV_VU);
  if (s_scope_line) {
    if (wave) {
      lv_obj_remove_flag(s_scope_line, LV_OBJ_FLAG_HIDDEN);
      lv_obj_remove_flag(s_scope_line2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_remove_flag(s_ov_db, LV_OBJ_FLAG_HIDDEN);
      if (s_ov_hold)
        lv_obj_remove_flag(s_ov_hold, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(s_scope_line, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(s_scope_line2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(s_ov_db, LV_OBJ_FLAG_HIDDEN);
      if (s_ov_hold)
        lv_obj_add_flag(s_ov_hold, LV_OBJ_FLAG_HIDDEN);
    }
  }
  if (s_ov_db)
    lv_obj_set_style_text_color(s_ov_db, current_theme.text_main, 0);
  s_ov_mode = mode;
  s_ov_start = lv_tick_get();
  s_ov_total = total_ms ? total_ms : 1;
  s_is_op_done = false;
  s_vu_display = 0;
  s_vu_hold = 0;
  if (s_ov_hold)
    lv_obj_set_pos(s_ov_hold, BAR_MARGIN, BAR_Y0);
  if (s_ov_timer == NULL)
    s_ov_timer = lv_timer_create(overlay_tick, OVERLAY_TICK_MS, NULL);
}

static void op_done_cb(void *unused) {
  (void)unused;
  s_is_op_done = true;
  s_state = ST_IDLE;
}

static void finish(const char *done_text) {
  strncpy(s_done_text, done_text, sizeof(s_done_text) - 1);
  s_done_text[sizeof(s_done_text) - 1] = '\0';
  ui_async_call(op_done_cb, NULL);
}

static void mic_level_cb(int peak, int rms, void *ctx) {
  (void)ctx;
  s_live_peak = peak;
  s_live_rms = rms;
  if (peak >= CLIP_THRESH)
    s_is_clipped = true;
  scope_push(peak);
}

static void wav_u16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
}

static void wav_u32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static void wav_fill_header(uint8_t *h, uint32_t data_len, uint32_t rate) {
  uint16_t block_align = WAV_CH_MONO * (WAV_BITS / 8);
  memcpy(h, "RIFF", 4);
  wav_u32(h + 4, (WAV_HDR_LEN - 8) + data_len);
  memcpy(h + 8, "WAVE", 4);
  memcpy(h + 12, "fmt ", 4);
  wav_u32(h + 16, WAV_FMT_CHUNK);
  wav_u16(h + 20, WAV_FMT_PCM);
  wav_u16(h + 22, WAV_CH_MONO);
  wav_u32(h + 24, rate);
  wav_u32(h + 28, rate * block_align);
  wav_u16(h + 32, block_align);
  wav_u16(h + 34, WAV_BITS);
  memcpy(h + 36, "data", 4);
  wav_u32(h + 40, data_len);
}

static void next_wav_path(char *out, size_t out_sz) {
  out[0] = '\0';
  for (int i = 1; i <= SAVE_MAX_IDX; i++) {
    snprintf(out, out_sz, "%s/rec_%03d.wav", REC_DIR, i);
    FILE *t = fopen(out, "rb");
    if (t == NULL)
      return;
    fclose(t);
  }
}

static void latest_wav_path(char *out, size_t out_sz) {
  out[0] = '\0';
  DIR *d = opendir(REC_DIR);
  if (d == NULL)
    return;
  int best = 0;
  struct dirent *e;
  while ((e = readdir(d)) != NULL) {
    int idx = 0;
    if (sscanf(e->d_name, "rec_%d.wav", &idx) == 1 && idx > best)
      best = idx;
  }
  closedir(d);
  if (best > 0)
    snprintf(out, out_sz, "%s/rec_%03d.wav", REC_DIR, best);
}

static void load_last_recording(void) {
  s_last_path[0] = '\0';
  s_last_ms = 0;
  latest_wav_path(s_last_path, sizeof(s_last_path));
  if (s_last_path[0] == '\0')
    return;
  FILE *f = fopen(s_last_path, "rb");
  if (f == NULL) {
    s_last_path[0] = '\0';
    return;
  }
  uint8_t h[WAV_HDR_LEN];
  if (fread(h, 1, WAV_HDR_LEN, f) == WAV_HDR_LEN) {
    uint32_t dlen = (uint32_t)h[40] | ((uint32_t)h[41] << 8) | ((uint32_t)h[42] << 16) |
                    ((uint32_t)h[43] << 24);
    s_last_ms = (uint32_t)((uint64_t)(dlen / (WAV_BITS / 8)) * 1000 / REC_RATE);
  }
  fclose(f);
}

static bool sd_has_space(uint64_t need_bytes) {
  vfs_statvfs_t st = {0};
  if (vfs_statvfs("/sdcard", &st) != ESP_OK)
    return true;
  return st.free_bytes >= need_bytes;
}

static void record_task(void *arg) {
  (void)arg;
  FILE *f = fopen(s_last_path, "wb");
  if (f == NULL) {
    s_last_path[0] = '\0';
    s_is_rec_ok = false;
    rec_finish("Save failed");
    vTaskDelete(NULL);
    return;
  }
  uint8_t hdr[WAV_HDR_LEN] = {0};
  fwrite(hdr, 1, WAV_HDR_LEN, f);

  uint32_t total = 0;
  bool disk_full = false;
  int16_t chunk[REC_CHUNK];
  if (audio_i2s_mic_stream_start(REC_RATE) == ESP_OK) {
    for (int w = 0; w < REC_WARMUP; w++)
      audio_i2s_mic_stream_read(chunk, REC_CHUNK);
    while (!s_is_stop_req && total < REC_MAX_SAMPLES) {
      int n = audio_i2s_mic_stream_read(chunk, REC_CHUNK);
      if (n <= 0)
        break;
      int32_t peak = 0;
      int64_t sq = 0;
      for (int i = 0; i < n; i++) {
        int32_t v = ((int32_t)chunk[i] * s_rec_gain_q8) >> 8;
        if (v > 32767)
          v = 32767;
        else if (v < -32768)
          v = -32768;
        chunk[i] = (int16_t)v;
        int32_t a = v < 0 ? -v : v;
        if (a > peak)
          peak = a;
        sq += (int64_t)v * v;
      }
      mic_level_cb((int)peak, (int)sqrtf((float)(sq / n)), NULL);
      if (s_is_paused)
        continue;
      if (fwrite(chunk, sizeof(int16_t), (size_t)n, f) != (size_t)n) {
        disk_full = true;
        break;
      }
      total += (uint32_t)n;
      s_rec_total = total;
    }
    audio_i2s_mic_stream_stop();
  }

  uint8_t h[WAV_HDR_LEN];
  wav_fill_header(h, total * (WAV_BITS / 8), REC_RATE);
  fseek(f, 0, SEEK_SET);
  fwrite(h, 1, WAV_HDR_LEN, f);
  fflush(f);
  fclose(f);

  s_last_ms = (uint32_t)((uint64_t)total * 1000 / REC_RATE);
  if (total == 0) {
    remove(s_last_path);
    s_last_path[0] = '\0';
    s_is_rec_ok = false;
    rec_finish("Mic failed");
  } else if (disk_full) {
    s_is_rec_ok = false;
    rec_finish("Disk full");
  } else {
    const char *base = strrchr(s_last_path, '/');
    char msg[24];
    snprintf(msg, sizeof(msg), "Saved %.17s", base ? base + 1 : s_last_path);
    s_is_rec_ok = true;
    rec_finish(msg);
  }
  vTaskDelete(NULL);
}

static void play_task(void *arg) {
  (void)arg;
  int16_t chunk[REC_CHUNK];
  bool played = false;
  do {
    FILE *f = fopen(s_last_path, "rb");
    if (f == NULL)
      break;
    fseek(f, WAV_HDR_LEN, SEEK_SET);
    if (audio_i2s_stream_start(REC_RATE) == ESP_OK) {
      while (!s_is_stop_req) {
        size_t got = fread(chunk, sizeof(int16_t), REC_CHUNK, f);
        if (got == 0)
          break;
        audio_i2s_stream_write(chunk, (int)got);
      }
      audio_i2s_stream_stop();
      played = true;
    }
    fclose(f);
  } while (s_is_loop && !s_is_stop_req);
  finish(played ? "Done" : "Play failed");
  s_is_busy = false;
  vTaskDelete(NULL);
}

static void activate(int idx) {
  if (s_is_busy)
    return;
  if (idx == ROW_REC) {
    if (!ui_sd_ready())
      return;
    if (!sd_has_space(REC_MIN_FREE)) {
      notify(NOTIFY_WARNING, "Disk full");
      return;
    }
    mkdir(REC_DIR, 0777);
    next_wav_path(s_last_path, sizeof(s_last_path));
    if (s_last_path[0] == '\0') {
      notify(NOTIFY_WARNING, "No free slot");
      return;
    }
    s_rec_gain_q8 = 256 << menu_component_get_intensity(&s_menu, ROW_LEVEL);
    s_live_peak = 0;
    s_live_rms = 0;
    s_is_clipped = false;
    s_vu_display = 0;
    s_vu_hold = 0;
    s_is_stop_req = false;
    s_is_paused = false;
    s_rec_total = 0;
    scope_reset();
    s_state = ST_RECORDING;
    s_is_busy = true;
    if (xTaskCreatePinnedToCore(
            record_task, "mic_rec", MIC_TASK_STACK, NULL, MIC_TASK_PRIORITY, NULL, SYS_CORE_RADIO) !=
        pdPASS) {
      s_is_busy = false;
      s_state = ST_IDLE;
      s_last_path[0] = '\0';
      notify(NOTIFY_WARNING, "Task error");
      return;
    }
    rec_screen_build();
  } else if (idx == ROW_PLAY) {
    if (s_last_path[0] == '\0') {
      overlay_show("Record first", 1, OV_TIME);
      finish("Record first");
      return;
    }
    s_is_loop = menu_component_get_toggle(&s_menu, ROW_LOOP);
    s_is_stop_req = false;
    s_state = ST_PLAYING;
    overlay_show(s_is_loop ? "PLAYING (loop)" : "PLAYING", s_last_ms + 150, OV_TIME);
    s_is_busy = true;
    if (xTaskCreatePinnedToCore(
            play_task, "mic_play", MIC_TASK_STACK, NULL, MIC_TASK_PRIORITY, NULL, SYS_CORE_RADIO) !=
        pdPASS) {
      s_is_busy = false;
      s_state = ST_IDLE;
      finish("Task error");
    }
  }
}

static void refresh_status(void) {
  if (s_status_lbl == NULL)
    return;
  char buf[40];
  if (s_state == ST_RECORDING)
    snprintf(buf, sizeof(buf), "RECORDING...");
  else if (s_state == ST_PLAYING)
    snprintf(buf, sizeof(buf), s_is_loop ? "PLAYING (loop)" : "PLAYING...");
  else if (s_last_path[0] != '\0')
    snprintf(buf,
             sizeof(buf),
             "IDLE   Last: %u.%us",
             (unsigned)(s_last_ms / 1000),
             (unsigned)((s_last_ms % 1000) / 100));
  else
    snprintf(buf, sizeof(buf), "IDLE   (no recording)");
  lv_label_set_text(s_status_lbl, buf);
}

static void micrec_tick_cb(lv_timer_t *t) {
  if (lv_screen_active() != s_screen) {
    lv_timer_delete(t);
    s_status_timer = NULL;
    return;
  }
  if (s_ov != NULL)
    return;

  refresh_status();
}

static void micrec_input(const input_event_t *ev, void *ctx) {
  (void)ctx;
  const bool press = (ev->action == INPUT_ACTION_PRESS);
  const bool nav = press || (ev->action == INPUT_ACTION_REPEAT);

  if (s_ov != NULL) {
    if (press && s_is_busy && s_state == ST_PLAYING && ev->button == INPUT_BTN_BACK)
      s_is_stop_req = true;
    return;
  }

  int sel = menu_component_get_selected(&s_menu);
  switch (ev->button) {
    case INPUT_BTN_DOWN:
      if (nav)
        menu_component_next(&s_menu);
      break;
    case INPUT_BTN_UP:
      if (nav)
        menu_component_prev(&s_menu);
      break;
    case INPUT_BTN_RIGHT:
      if (nav && sel == ROW_LEVEL)
        menu_component_intensity_inc(&s_menu, ROW_LEVEL);
      break;
    case INPUT_BTN_LEFT:
      if (nav && sel == ROW_LEVEL)
        menu_component_intensity_dec(&s_menu, ROW_LEVEL);
      break;
    case INPUT_BTN_OK:
      if (press) {
        if (sel == ROW_LOOP)
          menu_component_toggle_item(&s_menu, ROW_LOOP);
        else
          activate(sel);
      }
      break;
    case INPUT_BTN_BACK:
      if (press)
        ui_switch_screen(SCREEN_SETTINGS);
      break;
    default:
      break;
  }
}

static void rec_wf_redraw(void) {
  if (s_rec_wf == NULL)
    return;
  int head = s_scope_head;
  for (int j = 0; j < SCOPE_W; j++) {
    int idx = (head + j) % SCOPE_W;
    float v = (float)s_scope[idx] / SCOPE_FULL;
    if (v > 1.0f)
      v = 1.0f;
    int x = REC_WF_X0 + j * (REC_WF_X1 - REC_WF_X0) / (SCOPE_W - 1);
    int dy = (int)(v * (float)REC_WF_AMP);
    s_rec_pts[j].x = x;
    s_rec_pts[j].y = REC_WF_CY - dy;
    s_rec_pts2[j].x = x;
    s_rec_pts2[j].y = REC_WF_CY + dy;
  }
  lv_obj_invalidate(s_rec_wf);
  if (s_rec_wf2)
    lv_obj_invalidate(s_rec_wf2);
}

static void rec_screen_tick(lv_timer_t *t) {
  if (lv_screen_active() != s_rec_scr) {
    lv_timer_delete(t);
    s_rec_timer = NULL;
    return;
  }
  char tbuf[12];
  fmt_mmss(tbuf, sizeof(tbuf), (uint32_t)((uint64_t)s_rec_total * 1000 / REC_RATE));
  if (s_rec_time)
    lv_label_set_text(s_rec_time, tbuf);

  bool paused = s_is_paused;
  if (s_rec_state)
    lv_label_set_text(s_rec_state, paused ? "PAUSED" : "RECORDING");
  if (s_rec_dot) {
    if (paused) {
      lv_obj_set_style_bg_opa(s_rec_dot, LV_OPA_40, 0);
    } else {
      uint32_t ph = lv_tick_get() % 1000;
      uint32_t tri = ph < 500 ? ph : 1000 - ph;
      lv_obj_set_style_bg_opa(s_rec_dot, (lv_opa_t)(90 + tri * 165 / 500), 0);
    }
  }

  int pct = (int)((int64_t)s_live_peak * 100 / VU_FULLSCALE_PEAK);
  if (pct > 100)
    pct = 100;
  if (pct > s_vu_display)
    s_vu_display = pct;
  else
    s_vu_display = (s_vu_display * 7) / 10;
  if (s_rec_bar)
    lv_bar_set_value(s_rec_bar, s_vu_display, LV_ANIM_OFF);
  if (pct > s_vu_hold)
    s_vu_hold = pct;
  else if (s_vu_hold > 0)
    s_vu_hold -= VU_HOLD_DECAY_PCT;
  if (s_vu_hold < 0)
    s_vu_hold = 0;
  if (s_rec_hold)
    lv_obj_set_pos(s_rec_hold, REC_BAR_X0 + s_vu_hold * (REC_BAR_W - HOLD_W) / 100, REC_BAR_Y);
  if (s_rec_db) {
    if (s_is_clipped) {
      lv_obj_set_style_text_color(s_rec_db, lv_color_hex(ACCENT_RED), 0);
      lv_label_set_text(s_rec_db, "CLIP");
    } else {
      int rms = s_live_rms < 1 ? 1 : s_live_rms;
      int db = (int)(20.0f * log10f((float)rms / 32768.0f));
      char dbuf[16];
      snprintf(dbuf, sizeof(dbuf), "%d dBFS", db);
      lv_label_set_text(s_rec_db, dbuf);
    }
  }
  rec_wf_redraw();
}

static void rec_screen_input(const input_event_t *ev, void *ctx) {
  (void)ctx;
  if (ev->action != INPUT_ACTION_PRESS)
    return;
  if (ev->button == INPUT_BTN_OK) {
    s_is_paused = !s_is_paused;
    if (s_rec_state)
      lv_label_set_text(s_rec_state, s_is_paused ? "PAUSED" : "RECORDING");
  } else if (ev->button == INPUT_BTN_BACK) {
    s_is_stop_req = true;
  }
}

static void rec_screen_build(void) {
  s_rec_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s_rec_scr, current_theme.screen_base, 0);
  lv_obj_set_style_bg_opa(s_rec_scr, LV_OPA_COVER, 0);
  lv_obj_remove_flag(s_rec_scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_width(s_rec_scr, 0, 0);
  lv_obj_set_style_pad_all(s_rec_scr, 0, 0);

  ui_chrome_header(s_rec_scr, "RECORDER", "/assets/icons/mic.bin");
  ui_chrome_footer(s_rec_scr, "OK Pause   BACK Stop & Save");

  s_rec_dot = lv_obj_create(s_rec_scr);
  lv_obj_set_size(s_rec_dot, 12, 12);
  lv_obj_set_pos(s_rec_dot, 14, REC_TOP + 3);
  lv_obj_remove_flag(s_rec_dot, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(s_rec_dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(s_rec_dot, 0, 0);
  lv_obj_set_style_bg_color(s_rec_dot, lv_color_hex(ACCENT_RED), 0);

  s_rec_state = lv_label_create(s_rec_scr);
  lv_obj_set_style_text_font(s_rec_state, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_rec_state, lv_color_hex(ACCENT_RED), 0);
  lv_label_set_text(s_rec_state, "RECORDING");
  lv_obj_set_pos(s_rec_state, 34, REC_TOP);

  s_rec_time = lv_label_create(s_rec_scr);
  lv_obj_set_style_text_font(s_rec_time, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(s_rec_time, current_theme.text_main, 0);
  lv_label_set_text(s_rec_time, "0:00");
  lv_obj_align(s_rec_time, LV_ALIGN_TOP_RIGHT, -14, REC_TOP - 2);

  static lv_point_precise_t base_pts[2];
  base_pts[0].x = REC_WF_X0;
  base_pts[0].y = REC_WF_CY;
  base_pts[1].x = REC_WF_X1;
  base_pts[1].y = REC_WF_CY;
  lv_obj_t *base = lv_line_create(s_rec_scr);
  lv_line_set_points(base, base_pts, 2);
  lv_obj_set_style_line_width(base, 1, 0);
  lv_obj_set_style_line_color(base, current_theme.border_inactive, 0);

  for (int j = 0; j < SCOPE_W; j++) {
    int x = REC_WF_X0 + j * (REC_WF_X1 - REC_WF_X0) / (SCOPE_W - 1);
    s_rec_pts[j].x = x;
    s_rec_pts[j].y = REC_WF_CY;
    s_rec_pts2[j].x = x;
    s_rec_pts2[j].y = REC_WF_CY;
  }
  s_rec_wf = lv_line_create(s_rec_scr);
  lv_obj_set_style_line_width(s_rec_wf, 3, 0);
  lv_obj_set_style_line_color(s_rec_wf, lv_color_hex(ACCENT_RED), 0);
  lv_obj_set_style_line_rounded(s_rec_wf, true, 0);
  lv_line_set_points_mutable(s_rec_wf, s_rec_pts, SCOPE_W);
  s_rec_wf2 = lv_line_create(s_rec_scr);
  lv_obj_set_style_line_width(s_rec_wf2, 3, 0);
  lv_obj_set_style_line_color(s_rec_wf2, lv_color_hex(ACCENT_RED), 0);
  lv_obj_set_style_line_rounded(s_rec_wf2, true, 0);
  lv_line_set_points_mutable(s_rec_wf2, s_rec_pts2, SCOPE_W);

  s_rec_db = lv_label_create(s_rec_scr);
  lv_obj_set_style_text_font(s_rec_db, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(s_rec_db, current_theme.text_secondary, 0);
  lv_label_set_text(s_rec_db, "-- dBFS");
  lv_obj_set_pos(s_rec_db, REC_BAR_X0, REC_DB_Y);

  s_rec_bar = lv_bar_create(s_rec_scr);
  lv_obj_set_size(s_rec_bar, REC_BAR_W, 10);
  lv_obj_set_pos(s_rec_bar, REC_BAR_X0, REC_BAR_Y);
  lv_bar_set_range(s_rec_bar, 0, 100);
  lv_obj_set_style_bg_color(s_rec_bar, current_theme.bg_secondary, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(s_rec_bar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(s_rec_bar, 5, LV_PART_MAIN);
  lv_obj_set_style_bg_color(s_rec_bar, current_theme.border_accent, LV_PART_INDICATOR);
  lv_obj_set_style_radius(s_rec_bar, 5, LV_PART_INDICATOR);
  lv_bar_set_value(s_rec_bar, 0, LV_ANIM_OFF);

  s_rec_hold = lv_obj_create(s_rec_scr);
  lv_obj_set_size(s_rec_hold, HOLD_W, 10);
  lv_obj_set_pos(s_rec_hold, REC_BAR_X0, REC_BAR_Y);
  lv_obj_remove_flag(s_rec_hold, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_width(s_rec_hold, 0, 0);
  lv_obj_set_style_radius(s_rec_hold, 1, 0);
  lv_obj_set_style_bg_color(s_rec_hold, current_theme.text_main, 0);

  ui_input_set_screen_handler(rec_screen_input, NULL);
  lv_screen_load(s_rec_scr);
  s_rec_timer = lv_timer_create(rec_screen_tick, REC_TICK_MS, NULL);
}

static void rec_finish_cb(void *unused) {
  (void)unused;
  if (s_rec_timer != NULL) {
    lv_timer_delete(s_rec_timer);
    s_rec_timer = NULL;
  }
  s_state = ST_IDLE;
  s_is_paused = false;
  s_is_busy = false;
  lv_obj_t *old = s_rec_scr;
  s_rec_scr = NULL;
  if (s_screen != NULL) {
    lv_screen_load(s_screen);
    ui_input_set_screen_handler(micrec_input, NULL);
    if (s_status_timer == NULL)
      s_status_timer = lv_timer_create(micrec_tick_cb, STATUS_TICK_MS, NULL);
    refresh_status();
  }
  if (old != NULL)
    lv_obj_del(old);
  if (s_done_text[0] != '\0')
    notify(s_is_rec_ok ? NOTIFY_SAVED : NOTIFY_WARNING, s_done_text);
}

static void rec_finish(const char *msg) {
  strncpy(s_done_text, msg, sizeof(s_done_text) - 1);
  s_done_text[sizeof(s_done_text) - 1] = '\0';
  ui_async_call(rec_finish_cb, NULL);
}

void ui_micrec_open(void) {
  if (s_screen != NULL) {
    lv_obj_del(s_screen);
    s_screen = NULL;
  }
  s_ov = NULL;
  s_ov_dot = NULL;
  s_ov_state = NULL;
  s_ov_time = NULL;
  s_ov_db = NULL;
  s_ov_bar = NULL;
  s_ov_hold = NULL;
  s_scope_line = NULL;
  s_scope_line2 = NULL;
  s_ov_timer = NULL;
  s_is_clipped = false;
  s_rec_scr = NULL;
  s_rec_timer = NULL;
  s_is_paused = false;
  s_rec_total = 0;
  s_state = ST_IDLE;
  load_last_recording();

  s_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s_screen, current_theme.screen_base, 0);
  lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
  lv_obj_remove_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

  s_menu = menu_component_create(s_screen, "Recorder", "/assets/icons/mic.bin");
  menu_component_add_intensity(&s_menu, "/assets/icons/graphic_eq.bin", "Rec Level", 3);
  menu_component_add_item(&s_menu, "/assets/icons/fiber_manual_record.bin", "Record");
  menu_component_add_item(&s_menu, "/assets/icons/play_arrow.bin", "Play");
  menu_component_add_toggle(&s_menu, "/assets/icons/repeat.bin", "Loop", false);

  s_status_lbl = lv_label_create(s_screen);
  lv_label_set_text(s_status_lbl, "IDLE   (no recording)");
  lv_obj_set_style_text_color(s_status_lbl, current_theme.text_main, 0);
  lv_obj_set_style_text_opa(s_status_lbl, LV_OPA_70, 0);
  lv_obj_set_style_text_font(s_status_lbl, &lv_font_montserrat_12, 0);
  lv_obj_align(s_status_lbl, LV_ALIGN_BOTTOM_MID, 0, -4 - MENU_COMP_FOOTER_H);
  refresh_status();

  if (s_status_timer == NULL)
    s_status_timer = lv_timer_create(micrec_tick_cb, STATUS_TICK_MS, NULL);

  ui_input_set_screen_handler(micrec_input, NULL);

  ui_screen_load_owned(&s_screen, s_screen);
  ESP_LOGI(TAG, "mic-rec menu opened");
}
