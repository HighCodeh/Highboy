#ifndef IR_PATH_H
#define IR_PATH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "ff.h"

#ifdef __cplusplus
extern "C" {
#endif

// Configurações de path
#define MOUNT_POINT "/ir"
#define IR_PATH "/ir"
#define MAX_PATH_LEN 256
#define MAX_FILENAME_LEN 64
#define MAX_FILES_LIST 50

// Códigos de erro
typedef enum {
    IR_PATH_OK = 0,
    IR_PATH_ERR_INIT = -1,
    IR_PATH_ERR_MOUNT = -2,
    IR_PATH_ERR_CREATE_DIR = -3,
    IR_PATH_ERR_FILE_NOT_FOUND = -4,
    IR_PATH_ERR_WRITE_FAILED = -5,
    IR_PATH_ERR_READ_FAILED = -6,
    IR_PATH_ERR_DELETE_FAILED = -7,
    IR_PATH_ERR_INVALID_PARAM = -8,
    IR_PATH_ERR_MEMORY = -9,
    IR_PATH_ERR_PATH_TOO_LONG = -10
} ir_path_err_t;

// Estrutura para informações de arquivo
typedef struct {
    char name[MAX_FILENAME_LEN];
    size_t size;
    bool is_directory;
} file_info_t;

// Estrutura para lista de arquivos
typedef struct {
    file_info_t files[MAX_FILES_LIST];
    int count;
} file_list_t;

// Funções de inicialização
ir_path_err_t ir_path_init(void);
ir_path_err_t ir_path_deinit(void);

// Funções de arquivo
ir_path_err_t ir_path_write_file(const char* filename, const char* content, bool append);
ir_path_err_t ir_path_read_file(const char* filename, char* buffer, size_t buffer_size);
ir_path_err_t ir_path_delete_file(const char* filename);
ir_path_err_t ir_path_copy_file(const char* src_filename, const char* dst_filename);
ir_path_err_t ir_path_rename_file(const char* old_filename, const char* new_filename);
bool ir_path_file_exists(const char* filename);
ir_path_err_t ir_path_get_file_size(const char* filename, size_t* size);

// Funções de diretório
ir_path_err_t ir_path_create_directory(const char* dirname);
ir_path_err_t ir_path_delete_directory(const char* dirname);
ir_path_err_t ir_path_delete_directory_recursive(const char* dirname);
bool ir_path_directory_exists(const char* dirname);

// Funções de listagem
ir_path_err_t ir_path_list_files(file_list_t* file_list);

// Funções de informação
ir_path_err_t ir_path_get_storage_info(uint64_t* total_bytes, uint64_t* used_bytes);
const char* ir_path_error_to_string(ir_path_err_t error);

#ifdef __cplusplus
}
#endif

#endif // IR_PATH_H