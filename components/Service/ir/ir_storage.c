#include "ir_storage.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"

static const char *TAG = "IR_STORAGE";

bool ir_save(const char* protocol, uint32_t command, uint32_t address, const char* filename) {
    if (!protocol || !filename) {
        ESP_LOGE(TAG, "Parâmetros inválidos");
        return false;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/sdcard/%s.ir", filename);

    FILE* f = fopen(filepath, "w");
    if (!f) {
        ESP_LOGE(TAG, "Falha ao criar arquivo: %s", filepath);
        return false;
    }

    // Header do Flipper Zero
    fprintf(f, "Filetype: IR signals file\n");
    fprintf(f, "Version: 1\n");
    fprintf(f, "#\n");
    fprintf(f, "name: %s\n", filename);
    fprintf(f, "type: parsed\n");
    fprintf(f, "protocol: %s\n", protocol);
    fprintf(f, "address: %08lX\n", address);
    fprintf(f, "command: %08lX\n", command);

    fclose(f);

    ESP_LOGI(TAG, "Código IR salvo: %s -> %s (0x%08lX, 0x%08lX)", 
             filename, protocol, address, command);
    return true;
}

bool ir_load(const char* filename, ir_code_t* code) {
    if (!filename || !code) {
        ESP_LOGE(TAG, "Parâmetros inválidos");
        return false;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/sdcard/%s.ir", filename);

    FILE* f = fopen(filepath, "r");
    if (!f) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo: %s", filepath);
        return false;
    }

    char line[256];
    memset(code, 0, sizeof(ir_code_t));

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "protocol:", 9) == 0) {
            sscanf(line, "protocol: %31s", code->protocol);
        }
        else if (strncmp(line, "address:", 8) == 0) {
            sscanf(line, "address: %lX", &code->address);
        }
        else if (strncmp(line, "command:", 8) == 0) {
            sscanf(line, "command: %lX", &code->command);
        }
    }

    fclose(f);

    ESP_LOGI(TAG, "Código IR carregado: %s -> %s (0x%08lX, 0x%08lX)", 
             filename, code->protocol, code->address, code->command);
    return true;
}