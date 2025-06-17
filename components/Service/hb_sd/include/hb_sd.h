// hb_sd.h - Interface completa de gerenciamento de SD estilo Flipper Zero
#ifndef HB_SD_H
#define HB_SD_H

#include "esp_err.h"
#include "sdmmc_cmd.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Inicialização e estado
esp_err_t sdcard_init(void);
void sdcard_unmount(void);
bool sdcard_is_mounted(void);
void sdcard_print_info(void);
void sdcard_create_default_dirs(void);
sdmmc_card_t *hb_sd_get_card(void);

// Operações de arquivos e diretórios
bool sdcard_file_exists(const char *path);
esp_err_t sdcard_create_dir(const char *path);
esp_err_t sdcard_list_dir(const char *path);

// Escrita e leitura
esp_err_t sdcard_write_file(const char *path, const char *content);
esp_err_t sdcard_append_to_file(const char *path, const char *data);
esp_err_t sdcard_read_file_to_buffer(const char *path, char *buffer, size_t max_len);
esp_err_t sdcard_open_txt_file(const char *path);

// Manipulação de arquivos
esp_err_t sdcard_delete_file(const char *path);
esp_err_t sdcard_rename_file(const char *old_path, const char *new_path);
esp_err_t sdcard_copy_file(const char *src_path, const char *dest_path);

#ifdef __cplusplus
}
#endif

#endif // HB_SD_H
