// Copyright (c) 2025 HIGH CODE LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "st7789.h"
#include "home.h"
#include "storage_assets.h"
#include "esp_log.h"
#include <time.h>
#include <sys/time.h>

static const char *TAG = "home";

// Static buffers for bitmap assets
static uint8_t wifi_buf[48];      // 19x16 = 38 bytes
static uint8_t battery_buf[48];   // 24x16 = 48 bytes
static uint8_t sdcard_buf[16];    // 11x8 = 11 bytes
static uint8_t mhz_buf[48];       // 25x11 = 35 bytes
static uint8_t octo_buf[6300];    // 240x210 = 6300 bytes
static bool assets_loaded = false;

void home(void) {
    // Load assets on first call only
    if (!assets_loaded) {
        ESP_LOGI(TAG, "Loading home screen assets");
        
        if (storage_assets_read_file("icons/wifi_full.bin", wifi_buf, sizeof(wifi_buf), NULL) != ESP_OK ||
            storage_assets_read_file("icons/battery_charging.bin", battery_buf, sizeof(battery_buf), NULL) != ESP_OK ||
            storage_assets_read_file("icons/sdcard_mounted.bin", sdcard_buf, sizeof(sdcard_buf), NULL) != ESP_OK ||
            storage_assets_read_file("icons/mhz.bin", mhz_buf, sizeof(mhz_buf), NULL) != ESP_OK ||
            storage_assets_read_file("img/octo2.bin", octo_buf, sizeof(octo_buf), NULL) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to load assets");
            return;
        }
        
        assets_loaded = true;
        ESP_LOGI(TAG, "Assets loaded successfully");
    }
    
    // Clear framebuffer
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    
    // Set text size for subsequent calls
    st7789_set_text_size(2);
    
    // Draw UI shapes
    st7789_draw_round_rect_fb(8, 8, 224, 27, 8, 0x895F);  // Top rounded rectangle
    st7789_fill_circle_fb(22, 21, 4, 0x895F);             // Corner indicator
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    char time_str[6];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    
    st7789_set_text_size(1);
    st7789_draw_text_fb(35, 18, time_str, 0x895F, ST7789_COLOR_BLACK);
    st7789_set_text_size(2);
    
    st7789_draw_bitmap_fb(202, 13, wifi_buf, 19, 16, 0x895F);
    st7789_draw_bitmap_fb(171, 13, battery_buf, 24, 16, 0x895F);
    st7789_draw_bitmap_fb(151, 16, sdcard_buf, 11, 8, 0x895F);
    st7789_draw_bitmap_fb(117, 15, mhz_buf, 25, 11, 0x895F);
    
    // Draw main content
    st7789_draw_bitmap_fb(0, 30, octo_buf, 240, 210, 0x895F);
    
    // Flush framebuffer to display
    st7789_flush();
}