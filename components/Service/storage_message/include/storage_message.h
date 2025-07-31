#ifndef STORAGE_MESSAGE_H
#define STORAGE_MESSAGE_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_err.h"

#define MAX_FILES 32
#define MAX_FILENAME_LEN 64

// Estrutura para os resultados da listagem de arquivos
typedef struct {
    char names[MAX_FILES][MAX_FILENAME_LEN];
    int count;
    bool is_dir[MAX_FILES]; // Para saber se é um diretório
} file_list_t;

// Comandos que a API pode enviar para o Thread de Armazenamento
typedef enum {
    STORAGE_CMD_LIST_FILES,
    STORAGE_CMD_READ_FILE,
    STORAGE_CMD_WRITE_FILE
} StorageCommand;

// A estrutura da mensagem que vai na fila
typedef struct {
    StorageCommand command;       // O comando a ser executado
    SemaphoreHandle_t sync_sem;   // Um semáforo para sincronizar a resposta
    esp_err_t result;             // O resultado da operação (ESP_OK ou erro)

    // Dados de entrada e saída
    union {
        // Para STORAGE_CMD_LIST_FILES
        struct {
            const char* path;     // Entrada: caminho a ser listado
            file_list_t* list;    // Saída: ponteiro para preencher a lista
        } list_files;

        // Para STORAGE_CMD_READ_FILE
        struct {
            const char* path;      // Entrada: caminho do arquivo a ser lido
            char* buffer;          // Entrada: buffer pré-alocado para guardar o conteúdo
            size_t buffer_size;    // Entrada: tamanho do buffer
            size_t* bytes_read;    // Saída: quantos bytes foram realmente lidos
        } read_file;

        // Para STORAGE_CMD_WRITE_FILE
        struct {
            const char* path;      // Entrada: caminho do arquivo
            const void* data;      // Entrada: ponteiro para os dados a serem escritos
            size_t size;           // Entrada: tamanho dos dados
            bool append;           // Entrada: true para adicionar ao fim, false para sobrescrever
        } write_file;

    } payload;
} StorageMessage;

#endif // STORAGE_MESSAGE_H // <-- ESTA LINHA ESTAVA FALTANDO!