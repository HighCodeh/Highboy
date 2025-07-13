#include "ir_decoder.h"
#include <string.h>
#include "esp_log.h"

static const char* TAG = "IR_DECODER";

// Função para decodificar Sony SIRC
uint32_t decode_sony_sirc(const rmt_symbol_word_t* symbols, size_t count) {
    uint32_t data = 0;
    
    // Pula o header (primeiro símbolo)
    for (size_t i = 1; i < count; i++) {
        uint32_t mark_time = symbols[i].duration0;
        
        // Sony SIRC: 600us = '0', 1200us = '1' (aproximadamente)
        if (mark_time > 900) {  // > 900us consideramos '1'
            data |= (1 << (i - 1));
        }
        // Se <= 900us, consideramos '0' (não precisa setar)
    }
    
    return data;
}

// Função para decodificar RC6
uint32_t decode_rc6(const rmt_symbol_word_t* symbols, size_t count) {
    uint32_t data = 0;
    
    // RC6 tem codificação Manchester
    // Análise básica dos tempos
    for (size_t i = 1; i < count; i++) {
        uint32_t mark_time = symbols[i].duration0;
        uint32_t space_time = symbols[i].duration1;
        
        // Lógica simples baseada na duração do mark
        if (mark_time > 600) {  // Bit '1'
            data |= (1 << (i - 1));
        }
        // Se <= 600us, consideramos '0'
    }
    
    return data;
}

// Função para decodificar dados do protocolo
void decode_protocol_data(const char* protocol, const rmt_symbol_word_t* symbols, size_t count, ir_decoded_data_t* decoded) {
    // Copia o nome do protocolo
    strncpy(decoded->protocol, protocol, sizeof(decoded->protocol) - 1);
    decoded->protocol[sizeof(decoded->protocol) - 1] = '\0';
    
    // Decodifica baseado no protocolo
    if (strcmp(protocol, "NEC") == 0) {
        uint32_t data = decode_nec_data(symbols, count);
        
        // Para NEC, extrai address e command dos 32 bits
        decoded->address = (data >> 16) & 0xFFFF;  // 16 bits superiores
        decoded->command = data & 0xFFFF;          // 16 bits inferiores
        
        ESP_LOGI(TAG, "NEC - Address: 0x%04X, Command: 0x%04X", 
                 (unsigned int)decoded->address, (unsigned int)decoded->command);
                 
    } else if (strcmp(protocol, "RC5") == 0) {
        // Implementar decodificação RC5 se necessário
        decoded->address = 0x0000;  // Placeholder
        decoded->command = 0x0000;  // Placeholder
        ESP_LOGI(TAG, "RC5 - Decodificação não implementada ainda");
        
    } else if (strcmp(protocol, "SONY") == 0 || strcmp(protocol, "SIRC") == 0) {
        uint32_t data = decode_sony_sirc(symbols, count);
        
        // Sony SIRC: 7 bits = comando, 5 bits = endereço (formato 12 bits)
        // ou 7 bits = comando, 8 bits = endereço (formato 15 bits)
        decoded->command = data & 0x7F;         // 7 bits de comando
        decoded->address = (data >> 7) & 0x1F;  // 5 bits de endereço
        
        ESP_LOGI(TAG, "SONY/SIRC - Raw: 0x%08X, Address: 0x%02X, Command: 0x%02X", 
                 (unsigned int)data, (unsigned int)decoded->address, (unsigned int)decoded->command);
                 
    } else if (strcmp(protocol, "RC6") == 0) {
        uint32_t data = decode_rc6(symbols, count);
        
        // RC6: estrutura varia, mas geralmente 8 bits endereço + 8 bits comando
        decoded->address = (data >> 8) & 0xFF;  // 8 bits superiores
        decoded->command = data & 0xFF;         // 8 bits inferiores
        
        ESP_LOGI(TAG, "RC6 - Raw: 0x%08X, Address: 0x%02X, Command: 0x%02X", 
                 (unsigned int)data, (unsigned int)decoded->address, (unsigned int)decoded->command);
        
    } else {
        // Protocolo desconhecido - tenta decodificação genérica
        uint32_t data = 0;
        
        // Análise genérica baseada nos tempos
        for (size_t i = 1; i < count && i < 32; i++) {
            uint32_t mark_time = symbols[i].duration0;
            
            if (mark_time > 700) {  // Threshold genérico
                data |= (1 << (i - 1));
            }
        }
        
        decoded->address = (data >> 8) & 0xFF;
        decoded->command = data & 0xFF;
        
        ESP_LOGI(TAG, "Protocolo desconhecido (%s) - Raw: 0x%08X, Address: 0x%02X, Command: 0x%02X", 
                 protocol, (unsigned int)data, (unsigned int)decoded->address, (unsigned int)decoded->command);
    }
}