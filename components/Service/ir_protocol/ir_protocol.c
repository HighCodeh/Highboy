#include "ir_protocol.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "driver/rmt_rx.h"
#include <inttypes.h>

#define TAG "IR_PROTOCOL"

// Tolerância para comparação de tempos (em %)
#define TIME_TOLERANCE 20

// Estrutura para definir características de cada protocolo
typedef struct {
    const char* name;
    uint32_t header_mark_min;
    uint32_t header_mark_max;
    uint32_t header_space_min;
    uint32_t header_space_max;
    uint32_t bit_mark_min;
    uint32_t bit_mark_max;
    uint32_t bit_space_0_min;
    uint32_t bit_space_0_max;
    uint32_t bit_space_1_min;
    uint32_t bit_space_1_max;
    uint8_t expected_bits;
    bool has_header;
} ir_protocol_t;

// Definições dos protocolos mais comuns
static const ir_protocol_t protocols[] = {
    // NEC Protocol
    {
        .name = "NEC",
        .header_mark_min = 8500,
        .header_mark_max = 9500,
        .header_space_min = 4000,
        .header_space_max = 5000,
        .bit_mark_min = 400,
        .bit_mark_max = 700,
        .bit_space_0_min = 400,
        .bit_space_0_max = 700,
        .bit_space_1_min = 1500,
        .bit_space_1_max = 1800,
        .expected_bits = 32,
        .has_header = true
    },
    // Sony SIRC (12-bit)
    {
        .name = "SONY_SIRC_12",
        .header_mark_min = 2200,
        .header_mark_max = 2800,
        .header_space_min = 500,
        .header_space_max = 800,
        .bit_mark_min = 500,
        .bit_mark_max = 700,
        .bit_space_0_min = 400,
        .bit_space_0_max = 700,
        .bit_space_1_min = 1000,
        .bit_space_1_max = 1400,
        .expected_bits = 12,
        .has_header = true
    },
    // Sony SIRC (15-bit)
    {
        .name = "SONY_SIRC_15",
        .header_mark_min = 2200,
        .header_mark_max = 2800,
        .header_space_min = 500,
        .header_space_max = 800,
        .bit_mark_min = 500,
        .bit_mark_max = 700,
        .bit_space_0_min = 400,
        .bit_space_0_max = 700,
        .bit_space_1_min = 1000,
        .bit_space_1_max = 1400,
        .expected_bits = 15,
        .has_header = true
    },
    // Sony SIRC (20-bit)
    {
        .name = "SONY_SIRC_20",
        .header_mark_min = 2200,
        .header_mark_max = 2800,
        .header_space_min = 500,
        .header_space_max = 800,
        .bit_mark_min = 500,
        .bit_mark_max = 700,
        .bit_space_0_min = 400,
        .bit_space_0_max = 700,
        .bit_space_1_min = 1000,
        .bit_space_1_max = 1400,
        .expected_bits = 20,
        .has_header = true
    },
    // RC5 Protocol (Manchester encoding)
    {
        .name = "RC5",
        .header_mark_min = 0,
        .header_mark_max = 0,
        .header_space_min = 0,
        .header_space_max = 0,
        .bit_mark_min = 800,
        .bit_mark_max = 1000,
        .bit_space_0_min = 800,
        .bit_space_0_max = 1000,
        .bit_space_1_min = 800,
        .bit_space_1_max = 1000,
        .expected_bits = 14,
        .has_header = false
    },
    // RC6 Protocol
    {
        .name = "RC6",
        .header_mark_min = 2600,
        .header_mark_max = 2800,
        .header_space_min = 800,
        .header_space_max = 1000,
        .bit_mark_min = 350,
        .bit_mark_max = 500,
        .bit_space_0_min = 350,
        .bit_space_0_max = 500,
        .bit_space_1_min = 350,
        .bit_space_1_max = 500,
        .expected_bits = 20,
        .has_header = true
    },
    // Samsung Protocol
    {
        .name = "SAMSUNG",
        .header_mark_min = 4400,
        .header_mark_max = 4600,
        .header_space_min = 4400,
        .header_space_max = 4600,
        .bit_mark_min = 500,
        .bit_mark_max = 700,
        .bit_space_0_min = 500,
        .bit_space_0_max = 700,
        .bit_space_1_min = 1500,
        .bit_space_1_max = 1800,
        .expected_bits = 32,
        .has_header = true
    },
    // LG Protocol
    {
        .name = "LG",
        .header_mark_min = 8000,
        .header_mark_max = 9000,
        .header_space_min = 4000,
        .header_space_max = 5000,
        .bit_mark_min = 500,
        .bit_mark_max = 700,
        .bit_space_0_min = 500,
        .bit_space_0_max = 700,
        .bit_space_1_min = 1500,
        .bit_space_1_max = 1800,
        .expected_bits = 28,
        .has_header = true
    }
};

// Função para verificar se um tempo está dentro da faixa esperada
static bool time_in_range(uint32_t time, uint32_t min_time, uint32_t max_time) {
    return (time >= min_time && time <= max_time);
}

// Função para contar bits válidos baseado no protocolo
static int count_valid_bits(const rmt_symbol_word_t* symbols, size_t symbol_count, 
                           const ir_protocol_t* protocol, int start_index) {
    int valid_bits = 0;
    
    for (int i = start_index; i < symbol_count; i++) {
        uint32_t mark = symbols[i].duration0;
        uint32_t space = symbols[i].duration1;
        
        // Verifica se o mark está na faixa correta
        if (!time_in_range(mark, protocol->bit_mark_min, protocol->bit_mark_max)) {
            break;
        }
        
        // Verifica se o space corresponde a bit 0 ou bit 1
        bool is_bit_0 = time_in_range(space, protocol->bit_space_0_min, protocol->bit_space_0_max);
        bool is_bit_1 = time_in_range(space, protocol->bit_space_1_min, protocol->bit_space_1_max);
        
        if (is_bit_0 || is_bit_1) {
            valid_bits++;
        } else {
            break;
        }
    }
    
    return valid_bits;
}

// Função principal para detectar o protocolo IR
const char* detect_ir_protocol(const rmt_symbol_word_t* symbols, size_t symbol_count) {
    if (symbol_count < 2) {
        ESP_LOGW(TAG, "Muito poucos símbolos para análise (%d)", (int)symbol_count);
        return "UNKNOWN";
    }
    
    ESP_LOGI(TAG, "Analisando %d símbolos...", (int)symbol_count);
    
    // Mostra os primeiros símbolos para debug
    for (int i = 0; i < symbol_count && i < 5; i++) {
        ESP_LOGI(TAG, "Símbolo[%d]: Mark=%dus, Space=%dus", 
                 i, symbols[i].duration0, symbols[i].duration1);
    }
    
    const char* best_match = "UNKNOWN";
    int best_score = 0;
    
    // Testa cada protocolo
    for (int p = 0; p < sizeof(protocols) / sizeof(protocols[0]); p++) {
        const ir_protocol_t* protocol = &protocols[p];
        int score = 0;
        int start_bit_index = 0;
        
        ESP_LOGD(TAG, "Testando protocolo: %s", protocol->name);
        
        // Verifica header se o protocolo tem
        if (protocol->has_header) {
            uint32_t header_mark = symbols[0].duration0;
            uint32_t header_space = symbols[0].duration1;
            
            if (time_in_range(header_mark, protocol->header_mark_min, protocol->header_mark_max) &&
                time_in_range(header_space, protocol->header_space_min, protocol->header_space_max)) {
                score += 10; // Bonus por header correto
                start_bit_index = 1;
                ESP_LOGD(TAG, "Header válido para %s", protocol->name);
            } else {
                ESP_LOGD(TAG, "Header inválido para %s (mark=%" PRIu32 ", space=%" PRIu32 ")", protocol->name, header_mark, header_space);
                continue;
            }
        }
        
        // Conta bits válidos
        int valid_bits = count_valid_bits(symbols, symbol_count, protocol, start_bit_index);
        ESP_LOGD(TAG, "%s: %d bits válidos (esperado: %d)", 
                 protocol->name, valid_bits, protocol->expected_bits);
        
        // Calcula score baseado na proximidade com o número esperado de bits
        if (valid_bits > 0) {
            if (valid_bits == protocol->expected_bits) {
                score += 20; // Bonus por número exato de bits
            } else if (abs(valid_bits - protocol->expected_bits) <= 2) {
                score += 10; // Bonus por número próximo de bits
            } else {
                score += valid_bits; // Score baseado no número de bits válidos
            }
        }
        
        ESP_LOGD(TAG, "Score final para %s: %d", protocol->name, score);
        
        if (score > best_score) {
            best_score = score;
            best_match = protocol->name;
        }
    }
    
    // Análise especial para seu sinal específico
    if (symbol_count >= 2) {
        uint32_t first_mark = symbols[0].duration0;
        uint32_t first_space = symbols[0].duration1;
        
        // Verifica se pode ser um protocolo customizado ou variante
        if (first_mark > 2000 && first_space > 800) {
            if (strcmp(best_match, "UNKNOWN") == 0) {
                ESP_LOGI(TAG, "Possível protocolo customizado ou variante Sony/RC6");
                return "CUSTOM_SONY_LIKE";
            }
        }
    }
    
    ESP_LOGI(TAG, "Protocolo detectado: %s (score: %d)", best_match, best_score);
    return best_match;
}

// Função para decodificar dados do protocolo (exemplo para NEC)
uint32_t decode_nec_data(const rmt_symbol_word_t* symbols, size_t symbol_count) {
    if (symbol_count < 33) { // Header + 32 bits
        return 0;
    }
    
    uint32_t data = 0;
    
    // Pula o header (símbolo 0)
    for (int i = 1; i < 33 && i < symbol_count; i++) {
        uint32_t space = symbols[i].duration1;
        
        data <<= 1;
        
        // Bit 1 se space > 1000us, bit 0 se space < 1000us
        if (space > 1000) {
            data |= 1;
        }
    }
    
    return data;
}

// Função para análise detalhada do seu sinal específico
void analyze_your_signal(const rmt_symbol_word_t* symbols, size_t symbol_count) {
    ESP_LOGI(TAG, "=== ANÁLISE DETALHADA DO SEU SINAL ===");
    
    if (symbol_count < 1) return;
    
    // Analisa o primeiro símbolo (possível header)
    uint32_t first_mark = symbols[0].duration0;
    uint32_t first_space = symbols[0].duration1;
    
    ESP_LOGI(TAG, "Primeiro símbolo: Mark=%" PRIu32 "us, Space=%" PRIu32 "us", first_mark, first_space);
    
    if (first_mark > 2000) {
        ESP_LOGI(TAG, "Header detectado! Possível protocolo Sony SIRC ou similar");
        
        // Analisa os bits seguintes
        int bit_count = 0;
        for (int i = 1; i < symbol_count; i++) {
            uint32_t mark = symbols[i].duration0;
            uint32_t space = symbols[i].duration1;
            
            if (mark >= 350 && mark <= 500 && space >= 400 && space <= 1000) {
                bit_count++;
                ESP_LOGD(TAG, "Bit %d: %s (space=%" PRIu32 "us)", bit_count, (space > 700) ? "1" : "0", space);

            }
        }
        
        ESP_LOGI(TAG, "Total de bits válidos: %d", bit_count);
        
        if (bit_count == 12) {
            ESP_LOGI(TAG, "Provavelmente Sony SIRC 12-bit");
        } else if (bit_count == 15) {
            ESP_LOGI(TAG, "Provavelmente Sony SIRC 15-bit");
        } else if (bit_count == 20) {
            ESP_LOGI(TAG, "Provavelmente Sony SIRC 20-bit");
        }
    }
}
