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
 * @file storage_write.h
 * @brief Write Operations - Backend Agnostic
 * @version 1.0
 * 
 * Works with any backend (SD Card, SPIFFS, LittleFS, RAMFS)
 */

#ifndef STORAGE_WRITE_H
#define STORAGE_WRITE_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * STRING WRITE
 * ============================================================================ */

/**
 * @brief Write string to file (overwrite)
 * @param path File path (e.g., "config.txt")
 * @param data String to write
 * @return ESP_OK on success
 */
esp_err_t storage_write_string(const char *path, const char *data);

/**
 * @brief Append string to end of file
 * @param path File path
 * @param data String to append
 * @return ESP_OK on success
 */
esp_err_t storage_append_string(const char *path, const char *data);

/* ============================================================================
 * BINARY WRITE
 * ============================================================================ */

/**
 * @brief Write binary data (overwrite)
 * @param path File path
 * @param data Data to write
 * @param size Size in bytes
 * @return ESP_OK on success
 */
esp_err_t storage_write_binary(const char *path, const void *data, size_t size);

/**
 * @brief Append binary data to end
 * @param path File path
 * @param data Data to append
 * @param size Size in bytes
 * @return ESP_OK on success
 */
esp_err_t storage_append_binary(const char *path, const void *data, size_t size);

/* ============================================================================
 * LINE WRITE
 * ============================================================================ */

/**
 * @brief Write single line (adds \n automatically)
 * @param path File path
 * @param line Line to write
 * @return ESP_OK on success
 */
esp_err_t storage_write_line(const char *path, const char *line);

/**
 * @brief Append line to file (adds \n automatically)
 * @param path File path
 * @param line Line to append
 * @return ESP_OK on success
 */
esp_err_t storage_append_line(const char *path, const char *line);

/* ============================================================================
 * FORMATTED WRITE
 * ============================================================================ */

/**
 * @brief Write formatted text (printf style)
 * @param path File path
 * @param format Format string
 * @param ... Variable arguments
 * @return ESP_OK on success
 * 
 * Example: storage_write_formatted("log.txt", "Temp: %d°C\n", temp);
 */
esp_err_t storage_write_formatted(const char *path, const char *format, ...) 
    __attribute__((format(printf, 2, 3)));

/**
 * @brief Append formatted text
 * @param path File path
 * @param format Format string
 * @param ... Variable arguments
 * @return ESP_OK on success
 */
esp_err_t storage_append_formatted(const char *path, const char *format, ...) 
    __attribute__((format(printf, 2, 3)));

/* ============================================================================
 * SPECIFIC TYPES
 * ============================================================================ */

/**
 * @brief Write generic buffer
 * @param path File path
 * @param buffer Data buffer
 * @param size Buffer size
 * @return ESP_OK on success
 */
esp_err_t storage_write_buffer(const char *path, const void *buffer, size_t size);

/**
 * @brief Write byte array
 * @param path File path
 * @param bytes Byte array
 * @param count Number of bytes
 * @return ESP_OK on success
 */
esp_err_t storage_write_bytes(const char *path, const uint8_t *bytes, size_t count);

/**
 * @brief Write single byte
 * @param path File path
 * @param byte Byte to write
 * @return ESP_OK on success
 */
esp_err_t storage_write_byte(const char *path, uint8_t byte);

/**
 * @brief Write integer
 * @param path File path
 * @param value Integer value
 * @return ESP_OK on success
 */
esp_err_t storage_write_int(const char *path, int32_t value);

/**
 * @brief Write float
 * @param path File path
 * @param value Float value
 * @return ESP_OK on success
 */
esp_err_t storage_write_float(const char *path, float value);

/* ============================================================================
 * CSV WRITE
 * ============================================================================ */

/**
 * @brief Write CSV row (overwrites file)
 * @param path File path
 * @param columns String array (columns)
 * @param num_columns Number of columns
 * @return ESP_OK on success
 * 
 * Example:
 *   const char *cols[] = {"Name", "Age", "City"};
 *   storage_write_csv_row("data.csv", cols, 3);
 *   // Result: Name,Age,City\n
 */
esp_err_t storage_write_csv_row(const char *path, const char **columns, size_t num_columns);

/**
 * @brief Append CSV row
 * @param path File path
 * @param columns String array
 * @param num_columns Number of columns
 * @return ESP_OK on success
 */
esp_err_t storage_append_csv_row(const char *path, const char **columns, size_t num_columns);

#ifdef __cplusplus
}
#endif

#endif // STORAGE_WRITE_H