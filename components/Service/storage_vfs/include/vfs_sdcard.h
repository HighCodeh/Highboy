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

/**
 * @file vfs_sdcard.h
 * @brief SD card backend interface
 */
#ifndef VFS_SDCARD_H
#define VFS_SDCARD_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

/**
 * @brief Initialize and mount SD card
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t vfs_sdcard_init(void);

/**
 * @brief Unmount SD card
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t vfs_sdcard_deinit(void);

/**
 * @brief Check if SD card is mounted
 * @return true if mounted, false otherwise
 */
bool vfs_sdcard_is_mounted(void);

/**
 * @brief Print SD card information
 */
void vfs_sdcard_print_info(void);

/**
 * @brief Format SD card
 * @return ESP_OK on success, error code otherwise
 * @warning Erases all data
 */
esp_err_t vfs_sdcard_format(void);

/* ============================================================================
 * REGISTRATION FUNCTIONS (used by vfs_auto.c)
 * ============================================================================ */

/**
 * @brief Register SD card backend in VFS
 * @return ESP_OK on success, error code otherwise
 *
 * This function is called automatically by vfs_init_auto().
 */
esp_err_t vfs_register_sd_backend(void);

/**
 * @brief Unregister SD card backend from VFS
 * @return ESP_OK on success, error code otherwise
 *
 * This function is called automatically by vfs_deinit_auto().
 */
esp_err_t vfs_unregister_sd_backend(void);

#ifdef __cplusplus
}
#endif

#endif // VFS_SDCARD_H
