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
 * @file storage_info.h
 * @brief Storage information interface
 */

#ifndef STORAGE_INFO_H
#define STORAGE_INFO_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Storage information structure
 */
typedef struct {
    char backend_name[32];
    char mount_point[64];
    bool is_mounted;
    uint64_t total_bytes;
    uint64_t free_bytes;
    uint64_t used_bytes;
    uint32_t block_size;
} storage_info_t;

esp_err_t storage_get_info(storage_info_t *info);
void storage_print_info_detailed(void);

esp_err_t storage_get_free_space(uint64_t *free_bytes);
esp_err_t storage_get_total_space(uint64_t *total_bytes);
esp_err_t storage_get_used_space(uint64_t *used_bytes);
esp_err_t storage_get_usage_percent(float *percentage);

const char* storage_get_backend_type(void);
const char* storage_get_mount_point_str(void);

#ifdef __cplusplus
}
#endif

#endif // STORAGE_INFO_H