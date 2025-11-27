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
 * @file vfs_core.h
 * @brief Virtual File System abstraction layer
 */

#ifndef VFS_CORE_H
#define VFS_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES AND CONSTANTS
 * ============================================================================ */

#define VFS_MAX_PATH        256
#define VFS_MAX_NAME        64
#define VFS_MAX_BACKENDS    4
#define VFS_INVALID_FD      -1

/** Supported backend types */
typedef enum {
    VFS_BACKEND_NONE = 0,
    VFS_BACKEND_SD_FAT,
    VFS_BACKEND_SPIFFS,
    VFS_BACKEND_LITTLEFS,
    VFS_BACKEND_RAMFS,
} vfs_backend_type_t;

/** File open flags (POSIX-style) */
typedef enum {
    VFS_O_RDONLY = 0x01,
    VFS_O_WRONLY = 0x02,
    VFS_O_RDWR   = 0x03,
    VFS_O_CREAT  = 0x04,
    VFS_O_TRUNC  = 0x08,
    VFS_O_APPEND = 0x10,
    VFS_O_EXCL   = 0x20,
} vfs_open_flags_t;

/** Seek origin (POSIX-style) */
typedef enum {
    VFS_SEEK_SET = 0,
    VFS_SEEK_CUR = 1,
    VFS_SEEK_END = 2,
} vfs_seek_whence_t;

/** Entry type (file / directory) */
typedef enum {
    VFS_TYPE_FILE = 1,
    VFS_TYPE_DIR  = 2,
} vfs_entry_type_t;

/** File descriptor */
typedef int vfs_fd_t;

/** File or directory information */
typedef struct {
    char name[VFS_MAX_NAME];
    vfs_entry_type_t type;
    size_t size;
    time_t mtime;
    time_t ctime;
    bool is_hidden;
    bool is_readonly;
} vfs_stat_t;

/** Filesystem information */
typedef struct {
    uint64_t total_bytes;
    uint64_t free_bytes;
    uint64_t used_bytes;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t free_blocks;
} vfs_statvfs_t;

/** Directory handle */
typedef struct vfs_dir_s* vfs_dir_t;

/** Directory listing callback */
typedef void (*vfs_dir_callback_t)(const vfs_stat_t *entry, void *user_data);

/* ============================================================================
 * BACKEND OPERATIONS
 * ============================================================================ */

typedef struct vfs_backend_ops_s {
    esp_err_t (*init)(void);
    esp_err_t (*deinit)(void);
    bool (*is_mounted)(void);

    vfs_fd_t (*open)(const char *path, int flags, int mode);
    ssize_t (*read)(vfs_fd_t fd, void *buf, size_t size);
    ssize_t (*write)(vfs_fd_t fd, const void *buf, size_t size);
    off_t (*lseek)(vfs_fd_t fd, off_t offset, int whence);
    esp_err_t (*close)(vfs_fd_t fd);
    esp_err_t (*fsync)(vfs_fd_t fd);

    esp_err_t (*stat)(const char *path, vfs_stat_t *st);
    esp_err_t (*fstat)(vfs_fd_t fd, vfs_stat_t *st);
    esp_err_t (*rename)(const char *old_path, const char *new_path);
    esp_err_t (*unlink)(const char *path);
    esp_err_t (*truncate)(const char *path, off_t length);

    esp_err_t (*mkdir)(const char *path, int mode);
    esp_err_t (*rmdir)(const char *path);
    vfs_dir_t (*opendir)(const char *path);
    esp_err_t (*readdir)(vfs_dir_t dir, vfs_stat_t *entry);
    esp_err_t (*closedir)(vfs_dir_t dir);

    esp_err_t (*statvfs)(vfs_statvfs_t *stat);
} vfs_backend_ops_t;

/** Backend configuration */
typedef struct {
    vfs_backend_type_t type;
    const char *mount_point;
    const vfs_backend_ops_t *ops;
    void *private_data;
} vfs_backend_config_t;

/* ============================================================================
 * BACKEND MANAGEMENT API
 * ============================================================================ */

esp_err_t vfs_register_backend(const vfs_backend_config_t *config);
esp_err_t vfs_unregister_backend(const char *mount_point);
const vfs_backend_config_t* vfs_get_backend(const char *path);
size_t vfs_list_backends(const vfs_backend_config_t **backends, size_t max_count);

/* ============================================================================
 * FILE OPERATIONS API
 * ============================================================================ */

vfs_fd_t vfs_open(const char *path, int flags, int mode);
ssize_t vfs_read(vfs_fd_t fd, void *buf, size_t size);
ssize_t vfs_write(vfs_fd_t fd, const void *buf, size_t size);
off_t vfs_lseek(vfs_fd_t fd, off_t offset, int whence);
esp_err_t vfs_close(vfs_fd_t fd);
esp_err_t vfs_fsync(vfs_fd_t fd);

/* ============================================================================
 * METADATA API
 * ============================================================================ */

esp_err_t vfs_stat(const char *path, vfs_stat_t *st);
esp_err_t vfs_fstat(vfs_fd_t fd, vfs_stat_t *st);
esp_err_t vfs_rename(const char *old_path, const char *new_path);
esp_err_t vfs_unlink(const char *path);
esp_err_t vfs_truncate(const char *path, off_t length);
bool vfs_exists(const char *path);

/* ============================================================================
 * DIRECTORY API
 * ============================================================================ */

esp_err_t vfs_mkdir(const char *path, int mode);
esp_err_t vfs_rmdir(const char *path);
esp_err_t vfs_rmdir_recursive(const char *path);
vfs_dir_t vfs_opendir(const char *path);
esp_err_t vfs_readdir(vfs_dir_t dir, vfs_stat_t *entry);
esp_err_t vfs_closedir(vfs_dir_t dir);
esp_err_t vfs_list_dir(const char *path, vfs_dir_callback_t callback, void *user_data);

/* ============================================================================
 * FILESYSTEM INFORMATION API
 * ============================================================================ */

esp_err_t vfs_statvfs(const char *path, vfs_statvfs_t *stat);
esp_err_t vfs_get_free_space(const char *path, uint64_t *free_bytes);
esp_err_t vfs_get_total_space(const char *path, uint64_t *total_bytes);
esp_err_t vfs_get_usage_percent(const char *path, float *percentage);

/* ============================================================================
 * HIGH-LEVEL HELPERS
 * ============================================================================ */

esp_err_t vfs_read_file(const char *path, void *buf, size_t size, size_t *bytes_read);
esp_err_t vfs_write_file(const char *path, const void *buf, size_t size);
esp_err_t vfs_append_file(const char *path, const void *buf, size_t size);
esp_err_t vfs_copy_file(const char *src, const char *dst);
esp_err_t vfs_get_size(const char *path, size_t *size);

#ifdef __cplusplus
}
#endif

#endif // VFS_CORE_H
