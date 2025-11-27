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
 * @file vfs_littlefs.c
 * @brief VFS backend for LittleFS (flash)
 */

#include "vfs_core.h"
#include "vfs_config.h"
#include "vfs_littlefs.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

#ifdef VFS_USE_LITTLEFS

static const char *TAG = "vfs_littlefs";

// Backend state
static struct {
    bool mounted;
    size_t total_bytes;
    size_t used_bytes;
} s_littlefs = {0};

/* ============================================================================
 * BACKEND OPERATIONS IMPLEMENTATION
 * ============================================================================ */

// LittleFS uses standard POSIX file functions provided by ESP-IDF

static vfs_fd_t littlefs_open(const char *path, int flags, int mode)
{
    return open(path, flags, mode);
}

static ssize_t littlefs_read(vfs_fd_t fd, void *buf, size_t size)
{
    return read(fd, buf, size);
}

static ssize_t littlefs_write(vfs_fd_t fd, const void *buf, size_t size)
{
    return write(fd, buf, size);
}

static off_t littlefs_lseek(vfs_fd_t fd, off_t offset, int whence)
{
    return lseek(fd, offset, whence);
}

static esp_err_t littlefs_close(vfs_fd_t fd)
{
    return (close(fd) == 0) ? ESP_OK : ESP_FAIL;
}

static esp_err_t littlefs_fsync(vfs_fd_t fd)
{
    return (fsync(fd) == 0) ? ESP_OK : ESP_FAIL;
}

static esp_err_t littlefs_stat(const char *path, vfs_stat_t *st)
{
    struct stat native_stat;
    if (stat(path, &native_stat) != 0) {
        return ESP_FAIL;
    }
    
    const char *name = strrchr(path, '/');
    strncpy(st->name, name ? name + 1 : path, VFS_MAX_NAME - 1);
    st->name[VFS_MAX_NAME - 1] = '\0';
    
    st->type = S_ISDIR(native_stat.st_mode) ? VFS_TYPE_DIR : VFS_TYPE_FILE;
    st->size = native_stat.st_size;
    st->mtime = native_stat.st_mtime;
    st->ctime = native_stat.st_ctime;
    st->is_hidden = false;
    st->is_readonly = false;
    
    return ESP_OK;
}

static esp_err_t littlefs_fstat(vfs_fd_t fd, vfs_stat_t *st)
{
    struct stat native_stat;
    if (fstat(fd, &native_stat) != 0) {
        return ESP_FAIL;
    }
    
    st->name[0] = '\0';
    st->type = S_ISDIR(native_stat.st_mode) ? VFS_TYPE_DIR : VFS_TYPE_FILE;
    st->size = native_stat.st_size;
    st->mtime = native_stat.st_mtime;
    st->ctime = native_stat.st_ctime;
    st->is_hidden = false;
    st->is_readonly = false;
    
    return ESP_OK;
}

static esp_err_t littlefs_rename(const char *old_path, const char *new_path)
{
    return (rename(old_path, new_path) == 0) ? ESP_OK : ESP_FAIL;
}

static esp_err_t littlefs_unlink(const char *path)
{
    return (unlink(path) == 0) ? ESP_OK : ESP_FAIL;
}

static esp_err_t littlefs_truncate(const char *path, off_t length)
{
    return (truncate(path, length) == 0) ? ESP_OK : ESP_FAIL;
}

static esp_err_t littlefs_mkdir(const char *path, int mode)
{
    return (mkdir(path, mode) == 0) ? ESP_OK : ESP_FAIL;
}

static esp_err_t littlefs_rmdir(const char *path)
{
    return (rmdir(path) == 0) ? ESP_OK : ESP_FAIL;
}

static vfs_dir_t littlefs_opendir(const char *path)
{
    return (vfs_dir_t)opendir(path);
}

static esp_err_t littlefs_readdir(vfs_dir_t dir, vfs_stat_t *entry)
{
    DIR *d = (DIR *)dir;
    struct dirent *ent = readdir(d);
    
    if (!ent) {
        return ESP_ERR_NOT_FOUND;
    }
    
    strncpy(entry->name, ent->d_name, VFS_MAX_NAME - 1);
    entry->name[VFS_MAX_NAME - 1] = '\0';
    entry->type = (ent->d_type == DT_DIR) ? VFS_TYPE_DIR : VFS_TYPE_FILE;
    entry->size = 0;
    
    return ESP_OK;
}

static esp_err_t littlefs_closedir(vfs_dir_t dir)
{
    DIR *d = (DIR *)dir;
    return (closedir(d) == 0) ? ESP_OK : ESP_FAIL;
}

static esp_err_t littlefs_statvfs(vfs_statvfs_t *stat)
{
    if (!s_littlefs.mounted) {
        return ESP_ERR_INVALID_STATE;
    }
    
    size_t total, used;
    esp_err_t ret = esp_littlefs_info(VFS_PARTITION_LABEL, &total, &used);
    
    if (ret == ESP_OK) {
        stat->total_bytes = total;
        stat->used_bytes = used;
        stat->free_bytes = total - used;
        stat->block_size = 4096; // LittleFS uses 4 KB blocks
        stat->total_blocks = total / 4096;
        stat->free_blocks = (total - used) / 4096;
    }
    
    return ret;
}

static bool littlefs_is_mounted(void)
{
    return s_littlefs.mounted;
}

/* ============================================================================
 * BACKEND OPERATIONS TABLE
 * ============================================================================ */

static const vfs_backend_ops_t s_littlefs_ops = {
    .init = NULL,
    .deinit = NULL,
    .is_mounted = littlefs_is_mounted,
    
    .open = littlefs_open,
    .read = littlefs_read,
    .write = littlefs_write,
    .lseek = littlefs_lseek,
    .close = littlefs_close,
    .fsync = littlefs_fsync,
    
    .stat = littlefs_stat,
    .fstat = littlefs_fstat,
    .rename = littlefs_rename,
    .unlink = littlefs_unlink,
    .truncate = littlefs_truncate,
    
    .mkdir = littlefs_mkdir,
    .rmdir = littlefs_rmdir,
    .opendir = littlefs_opendir,
    .readdir = littlefs_readdir,
    .closedir = littlefs_closedir,
    
    .statvfs = littlefs_statvfs,
};

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

esp_err_t vfs_littlefs_init(void)
{
    if (s_littlefs.mounted) {
        ESP_LOGW(TAG, "LittleFS already mounted");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing LittleFS");
    ESP_LOGI(TAG, "Partition: %s", VFS_PARTITION_LABEL);
    ESP_LOGI(TAG, "Mount point: %s", VFS_MOUNT_POINT);
    
    esp_vfs_littlefs_conf_t conf = {
        .base_path = VFS_MOUNT_POINT,
        .partition_label = VFS_PARTITION_LABEL,
        .format_if_mount_failed = VFS_FORMAT_ON_FAIL,
        .dont_mount = false,
    };
    
    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount LittleFS: %s (0x%x)", esp_err_to_name(ret), ret);
        
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Partition not found, check partition table");
        }
        
        return ret;
    }
    
    s_littlefs.mounted = true;
    
    // Fetch partition info
    esp_littlefs_info(VFS_PARTITION_LABEL, &s_littlefs.total_bytes, &s_littlefs.used_bytes);
    
    // Register in VFS core
    vfs_backend_config_t backend_config = {
        .type = VFS_BACKEND_LITTLEFS,
        .mount_point = VFS_MOUNT_POINT,
        .ops = &s_littlefs_ops,
        .private_data = &s_littlefs,
    };
    
    ret = vfs_register_backend(&backend_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register LittleFS backend in VFS core");
        vfs_littlefs_deinit();
        return ret;
    }
    
    ESP_LOGI(TAG, "LittleFS mounted");
    ESP_LOGI(TAG, "Partition: %s", VFS_PARTITION_LABEL);
    ESP_LOGI(TAG, "Capacity: %u KB", s_littlefs.total_bytes / 1024);
    ESP_LOGI(TAG, "Used: %u KB", s_littlefs.used_bytes / 1024);
    ESP_LOGI(TAG, "Free: %u KB", (s_littlefs.total_bytes - s_littlefs.used_bytes) / 1024);
    
    return ESP_OK;
}

esp_err_t vfs_littlefs_deinit(void)
{
    if (!s_littlefs.mounted) {
        ESP_LOGW(TAG, "LittleFS is not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Unmounting LittleFS");
    
    // Unregister from VFS core first
    vfs_unregister_backend(VFS_MOUNT_POINT);
    
    esp_err_t ret = esp_vfs_littlefs_unregister(VFS_PARTITION_LABEL);
    
    if (ret == ESP_OK) {
        s_littlefs.mounted = false;
        s_littlefs.total_bytes = 0;
        s_littlefs.used_bytes = 0;
        ESP_LOGI(TAG, "LittleFS unmounted");
    } else {
        ESP_LOGE(TAG, "Failed to unmount LittleFS: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

bool vfs_littlefs_is_mounted(void)
{
    return s_littlefs.mounted;
}

/* ============================================================================
 * REGISTRATION HELPERS
 * ============================================================================ */

esp_err_t vfs_register_littlefs_backend(void)
{
    ESP_LOGI(TAG, "Registering LittleFS backend");
    return vfs_littlefs_init();
}

esp_err_t vfs_unregister_littlefs_backend(void)
{
    ESP_LOGI(TAG, "Unregistering LittleFS backend");
    return vfs_littlefs_deinit();
}

/* ============================================================================
 * AUXILIARY FUNCTIONS
 * ============================================================================ */

void vfs_littlefs_print_info(void)
{
    if (!s_littlefs.mounted) {
        ESP_LOGW(TAG, "LittleFS is not mounted");
        return;
    }
    
    ESP_LOGI(TAG, "LittleFS info:");
    ESP_LOGI(TAG, "Mount point: %s", VFS_MOUNT_POINT);
    ESP_LOGI(TAG, "Partition: %s", VFS_PARTITION_LABEL);
    
    size_t total, used;
    if (esp_littlefs_info(VFS_PARTITION_LABEL, &total, &used) == ESP_OK) {
        float percent = total > 0 ? ((float)used / total) * 100.0f : 0.0f;
        
        ESP_LOGI(TAG, "Total: %u KB (%.2f MB)", 
                 total / 1024, (float)total / (1024 * 1024));
        ESP_LOGI(TAG, "Used: %u KB (%.2f MB)", 
                 used / 1024, (float)used / (1024 * 1024));
        ESP_LOGI(TAG, "Free: %u KB (%.2f MB)", 
                 (total - used) / 1024, (float)(total - used) / (1024 * 1024));
        ESP_LOGI(TAG, "Usage: %.1f%%", percent);
    }
}

esp_err_t vfs_littlefs_format(void)
{
    if (!s_littlefs.mounted) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGW(TAG, "Formatting LittleFS");
    
    // Unmount
    esp_err_t ret = vfs_littlefs_deinit();
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Format partition
    ret = esp_littlefs_format(VFS_PARTITION_LABEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format LittleFS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Remount
    ret = vfs_littlefs_init();
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "LittleFS formatted successfully");
    }
    
    return ret;
}

#endif // VFS_USE_LITTLEFS
