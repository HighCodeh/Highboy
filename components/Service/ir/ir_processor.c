#include "ir_processor.h"
#include "ir_decoder.h"
#include "ir_file_generator.h"
#include "ir_debug.h"
#include "ir_protocol.h"
#include "ir_path.h"
#include "esp_log.h"

static const char* TAG = "IR_PROCESSOR";

// Função principal para processar sinais IR
void process_ir_signal(const rmt_symbol_word_t* symbols, size_t count) {
    ESP_LOGI(TAG, "Processando %d símbolos IR", (int)count);
    
    // DETECÇÃO DO PROTOCOLO
    const char* protocol = detect_ir_protocol(symbols, count);
    ESP_LOGI(TAG, "PROTOCOLO DETECTADO: %s", protocol);
    
    // ANÁLISE DETALHADA
    analyze_your_signal(symbols, count);
    
    // DECODIFICAÇÃO DOS DADOS
    ir_decoded_data_t decoded_data;
    decode_protocol_data(protocol, symbols, count, &decoded_data);
    
    // DEBUG: Mostra símbolos e dados decodificados
    debug_show_symbols(symbols, count, 10);
    debug_show_decoded_info(&decoded_data);
    
    // SALVAR ARQUIVO
    if (save_ir_file(protocol, &decoded_data)) {
        ESP_LOGI(TAG, "Processamento completo com sucesso!");
    } else {
        ESP_LOGE(TAG, "Erro no processamento do sinal");
    }
}

// Função para salvar arquivo .ir
bool save_ir_file(const char* protocol, const ir_decoded_data_t* decoded) {
    char filename[64];
    char signal_name[64];
    
    // Gera nome do arquivo e do sinal
    generate_filename(filename, sizeof(filename), protocol);
    generate_signal_name(signal_name, sizeof(signal_name), protocol, decoded);
    
    // Gera conteúdo do arquivo
    char* file_content = generate_ir_file_content(signal_name, decoded);
    
    // Salva o arquivo
    ESP_LOGI(TAG, "Salvando arquivo: %s", filename);
    ESP_LOGI(TAG, "Nome do sinal: %s", signal_name);
    
    if (ir_path_write_file(filename, file_content, false) == IR_PATH_OK) {
        ESP_LOGI(TAG, "Arquivo .ir salvo com sucesso!");
        
        // Verifica o arquivo salvo
        debug_verify_saved_file(filename, file_content);
        
        return true;
    } else {
        ESP_LOGE(TAG, "Erro ao salvar arquivo .ir");
        return false;
    }
}