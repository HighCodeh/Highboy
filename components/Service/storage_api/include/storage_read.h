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
 * @file storage_read.h
 * @brief Read Operations - Backend Agnostic
 * @version 1.0
 */

#ifndef STORAGE_READ_H
#define STORAGE_READ_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * STRING READ
 * ============================================================================ */

/**
 * @brief Read string from file
 * @param path File path
 * @param buffer Destination buffer
 * @param buffer_size Buffer size
 * @return ESP_OK on success
 * 
 * Automatically adds null terminator
 */
esp_err_t storage_read_string(const char *path, char *buffer, size_t buffer_size);

/* ============================================================================
 * BINARY READ
 * ============================================================================ */

/**
 * @brief Read binary data
 * @param path File path
 * @param buffer Destination buffer
 * @param size Bytes to read
 * @param bytes_read Actual bytes read (optional)
 * @return ESP_OK on success
 */
esp_err_t storage_read_binary(const char *path, void *buffer, size_t size, size_t *bytes_read);

/* ============================================================================
 * LINE READ
 * ============================================================================ */

/**
 * @brief Callback to process lines
 * @param line Line read (without \n)
 * @param user_data User data
 */
typedef void (*storage_line_callback_t)(const char *line, void *user_data);

/**
 * @brief Read specific line from file
 * @param path File path
 * @param buffer Destination buffer
 * @param buffer_size Buffer size
 * @param line_number Line number (starts at 1)
 * @return ESP_OK on success
 */
esp_err_t storage_read_line(const char *path, char *buffer, size_t buffer_size, uint32_t line_number);

/**
 * @brief Read first line
 * @param path File path
 * @param buffer Destination buffer
 * @param buffer_size Buffer size
 * @return ESP_OK on success
 */
esp_err_t storage_read_first_line(const char *path, char *buffer, size_t buffer_size);

/**
 * @brief Read last line
 * @param path File path
 * @param buffer Destination buffer
 * @param buffer_size Buffer size
 * @return ESP_OK on success
 */
esp_err_t storage_read_last_line(const char *path, char *buffer, size_t buffer_size);

/**
 * @brief Process all lines with callback
 * @param path File path
 * @param callback Callback function
 * @param user_data User data
 * @return ESP_OK on success
 */
esp_err_t storage_read_lines(const char *path, storage_line_callback_t callback, void *user_data);

/**
 * @brief Count number of lines
 * @param path File path
 * @param line_count Line count output
 * @return ESP_OK on success
 */
esp_err_t storage_count_lines(const char *path, uint32_t *line_count);

/* ============================================================================
 * CHUNK READ
 * ============================================================================ */

/**
 * @brief Read specific chunk from file
 * @param path File path
 * @param offset Start offset
 * @param buffer Destination buffer
 * @param size Bytes to read
 * @param bytes_read Bytes read (optional)
 * @return ESP_OK on success
 */
esp_err_t storage_read_chunk(const char *path, size_t offset, void *buffer, size_t size, size_t *bytes_read);

/* ============================================================================
 * SPECIFIC TYPES
 * ============================================================================ */

/**
 * @brief Read integer
 * @param path File path
 * @param value Value output
 * @return ESP_OK on success
 */
esp_err_t storage_read_int(const char *path, int32_t *value);

/**
 * @brief Read float
 * @param path File path
 * @param value Value output
 * @return ESP_OK on success
 */
esp_err_t storage_read_float(const char *path, float *value);

/**
 * @brief Read bytes
 * @param path File path
 * @param bytes Destination buffer
 * @param max_count Maximum size
 * @param count Bytes read (optional)
 * @return ESP_OK on success
 */
esp_err_t storage_read_bytes(const char *path, uint8_t *bytes, size_t max_count, size_t *count);

/**
 * @brief Read single byte
 * @param path File path
 * @param byte Byte output
 * @return ESP_OK on success
 */
esp_err_t storage_read_byte(const char *path, uint8_t *byte);

/* ============================================================================
 * FILE SEARCH
 * ============================================================================ */

/**
 * @brief Check if file contains text
 * @param path File path
 * @param search Text to search
 * @param found Output (true if found)
 * @return ESP_OK on success
 */
esp_err_t storage_file_contains(const char *path, const char *search, bool *found);

/**
 * @brief Count text occurrences
 * @param path File path
 * @param search Text to search
 * @param count Count output
 * @return ESP_OK on success
 */
esp_err_t storage_count_occurrences(const char *path, const char *search, uint32_t *count);

#ifdef __cplusplus
}
#endif

#endif // STORAGE_READ_H