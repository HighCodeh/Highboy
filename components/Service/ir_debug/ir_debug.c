#include "ir_debug.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "ir_path.h"

static const char* TAG = "IR_DEBUG";

// Função para exibir primeiros símbolos para debug
void debug_show_symbols(const rmt_symbol_word_t* symbols, size_t count, size_t max_symbols) {
    ESP_LOGI(TAG, "=== SÍMBOLOS RECEBIDOS ===");
    size_t limit = (count < max_symbols) ? count : max_symbols;
    
    for (size_t i = 0; i < limit; i++) {
        ESP_LOGI(TAG, "Simb[%d] L0=%d D0=%dus L1=%d D1=%dus", (int)i,
                 symbols[i].level0, symbols[i].duration0,
                 symbols[i].level1, symbols[i].duration1);
    }
}

// Função para exibir informações decodificadas
void debug_show_decoded_info(const ir_decoded_data_t* decoded) {
    ESP_LOGI(TAG, "=== DADOS DECODIFICADOS ===");
    ESP_LOGI(TAG, "📄 Protocolo: %s", decoded->protocol);
    ESP_LOGI(TAG, "📄 Endereço: 0x%08X", (unsigned int)decoded->address);
    ESP_LOGI(TAG, "📄 Comando: 0x%08X", (unsigned int)decoded->command);
}

// Função para verificar arquivo salvo
void debug_verify_saved_file(const char* filename, const char* original_content) {
    ESP_LOGI(TAG, "🔍 Verificando arquivo salvo: %s", filename);
    char read_buffer[1024];
    
    if (ir_path_read_file(filename, read_buffer, sizeof(read_buffer)) == IR_PATH_OK) {
        ESP_LOGI(TAG, "✅ Arquivo lido com sucesso!");
        ESP_LOGI(TAG, "📖 === CONTEÚDO LIDO DO ARQUIVO ===");
        printf("%s", read_buffer);
        ESP_LOGI(TAG, "📖 === FIM DO ARQUIVO LIDO ===");
        
        // Verifica se o conteúdo lido é igual ao escrito
        if (strcmp(original_content, read_buffer) == 0) {
            ESP_LOGI(TAG, "✅ Verificação OK: Conteúdo escrito = Conteúdo lido");
        } else {
            ESP_LOGW(TAG, "⚠️ Diferença detectada entre escrito e lido!");
            ESP_LOGW(TAG, "Tamanho escrito: %d, Tamanho lido: %d", 
                     strlen(original_content), strlen(read_buffer));
        }
    } else {
        ESP_LOGE(TAG, "❌ Erro ao ler arquivo para verificação");
    }
}