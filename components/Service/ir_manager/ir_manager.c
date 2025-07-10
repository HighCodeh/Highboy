#include "ir_transmitter.h"
#include "ir_receiver.h"
#include "driver/rmt_tx.h" 
#include "driver/rmt_rx.h"
#include "esp_log.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ir_protocol.h"
#include "ir_encoder.h"
#include "ir_file.h"
#include "ir_path.h"

#define TAG "MANAGER"

// Contador global para nomes únicos
static int ir_file_counter = 1;

static bool has_manager_init = false;

typedef struct {
    rmt_symbol_word_t *raw_symbols;
    size_t received;
    char *filename;
    char *signal_name;
    char *file_content;
    ir_decoded_data_t decoded_data;
} ir_reception_data_t;

bool ir_manager_init(){

    if(!rmt_tx_init() || !rmt_rx_init()){
        has_manager_init = false;
        return false;
    }

    has_manager_init = true;
    return true;

}

bool ir_manager_delete(){

    if(!rmt_tx_delete() || !rmt_rx_delete()){
        has_manager_init = true;
        return false;
    }

    has_manager_init = false;
    return true;
}

// - RECEIVE

void generate_ir_filename(char* filename, size_t max_len, const char* protocol) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    snprintf(filename, max_len, "IR_%s_%04d%02d%02d_%02d%02d%02d_%03d.ir",
             protocol,
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec,
             ir_file_counter++);
}

// Função para capturar output do printf e salvá-lo
static char* captured_output = NULL;
static size_t captured_size = 0;
static size_t captured_capacity = 0;

int printf_capture(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Calcula o tamanho necessário
    int needed = vsnprintf(NULL, 0, format, args);
    va_end(args);
    
    if (needed < 0) return -1;
    
    // Verifica se precisa expandir o buffer
    if (captured_size + needed + 1 > captured_capacity) {
        size_t new_capacity = captured_capacity * 2;
        if (new_capacity < captured_size + needed + 1) {
            new_capacity = captured_size + needed + 1;
        }
        
        char* new_buffer = realloc(captured_output, new_capacity);
        if (!new_buffer) return -1;
        
        captured_output = new_buffer;
        captured_capacity = new_capacity;
    }
    
    // Escreve no buffer
    va_start(args, format);
    int written = vsnprintf(captured_output + captured_size, 
                           captured_capacity - captured_size, format, args);
    va_end(args);
    
    if (written > 0) {
        captured_size += written;
    }
    
    return written;
}

// Inicializa captura de output
void start_output_capture() {
    if (captured_output) {
        free(captured_output);
    }
    captured_capacity = 4096; // Buffer inicial de 4KB
    captured_output = malloc(captured_capacity);
    captured_size = 0;
    if (captured_output) {
        captured_output[0] = '\0';
    }
}

// Finaliza captura e retorna o conteúdo
char* finish_output_capture() {
    return captured_output; // Caller deve fazer free()
}

size_t ir_manager_receive(rmt_symbol_word_t *raw_symbols) {

    size_t received = 0;

    if(!has_manager_init){
        ESP_LOGE(TAG, "IR MANAGER NÃO INICIALIZADO");
        return received;
    }

    if (!rmt_rx_receive_start_queue()) {
        ESP_LOGE(TAG, "FALHA AO CRIAR FILA DE RECEPÇÃO");
        return received;
    }

    if (rmt_rx_receive_once(raw_symbols, MEM_BLOCK_SYMBOLS, &received)) {
        ESP_LOGI(TAG, "📡 Recebido %d símbolos IR", (int)received);
        
        // Log dos primeiros símbolos para debug
        for (size_t i = 0; i < received && i < 5; i++) {
            ESP_LOGI(TAG, "Simb[%d] L0=%d D0=%dus L1=%d D1=%dus", (int)i,
                     raw_symbols[i].level0, raw_symbols[i].duration0,
                     raw_symbols[i].level1, raw_symbols[i].duration1);
        }
        
        // DETECÇÃO DO PROTOCOLO
        const char* protocol = detect_ir_protocol(raw_symbols, received);
        ESP_LOGI(TAG, "🎯 PROTOCOLO DETECTADO: %s", protocol);
        
        // GERA NOME DO SINAL BASEADO NO PROTOCOLO E TIMESTAMP
        char signal_name[64];
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        
        snprintf(signal_name, sizeof(signal_name), "%s_Signal_%02d%02d%02d_%03d",
                 protocol,
                 timeinfo.tm_hour,
                 timeinfo.tm_min,
                 timeinfo.tm_sec,
                 ir_file_counter);
        
        // INICIA CAPTURA DO OUTPUT
        start_output_capture();
        
        // Substitui temporariamente printf para capturar saída
        #define printf printf_capture
        
        // GERA O ARQUIVO .IR USANDO A FUNÇÃO EXISTENTE
        ir_file_auto_print(signal_name, raw_symbols, received);
        
        // Restaura printf original
        #undef printf
        
        // OBTÉM O CONTEÚDO CAPTURADO
        char* ir_content = finish_output_capture();
        
        if (ir_content && strlen(ir_content) > 0) {
            // GERA NOME DO ARQUIVO
            char filename[128];
            generate_ir_filename(filename, sizeof(filename), protocol);
            
            // SALVA O ARQUIVO .IR NO SD CARD usando ir_path
            ir_path_err_t result = ir_path_write_file(filename, ir_content, false);
            
            if (result == IR_PATH_OK) {
                ESP_LOGI(TAG, "✅ Arquivo .ir salvo com sucesso no SD Card: %s", filename);
                ESP_LOGI(TAG, "📋 Nome do sinal: %s", signal_name);
                ESP_LOGI(TAG, "🏷️  Protocolo: %s", protocol);
                
                // OPCIONAL: Exibe o conteúdo do arquivo para debug
                ESP_LOGI(TAG, "📄 Conteúdo do arquivo .ir:");
                printf("%s", ir_content); // Usa printf original aqui
                
                // OPCIONAL: Testa o parsing do arquivo recém-criado
                ir_file_data_t parsed_data;
                if (ir_file_parse_string(ir_content, &parsed_data)) {
                    ESP_LOGI(TAG, "✅ Arquivo .ir validado com sucesso!");
                    ESP_LOGI(TAG, "   Nome: %s", parsed_data.name);
                    ESP_LOGI(TAG, "   Tipo: %s", parsed_data.type);
                    ESP_LOGI(TAG, "   Protocolo: %s", parsed_data.protocol);
                    
                    if (strcmp(parsed_data.type, "parsed") == 0) {
                        ESP_LOGI(TAG, "   Endereço: 0x%08lX", (unsigned long)parsed_data.address);
                        ESP_LOGI(TAG, "   Comando: 0x%08lX", (unsigned long)parsed_data.command);
                    } else {
                        ESP_LOGI(TAG, "   Frequência: %lu Hz", (unsigned long)parsed_data.frequency);
                        ESP_LOGI(TAG, "   Dados RAW: %zu valores", parsed_data.raw_data_count);
                    }
                    
                    ir_file_free_data(&parsed_data);
                } else {
                    ESP_LOGW(TAG, "⚠️  Falha na validação do arquivo .ir gerado");
                }
                
            } else {
                ESP_LOGE(TAG, "❌ Falha ao salvar arquivo .ir no SD Card: %s", 
                         ir_path_error_to_string(result));
            }
        } else {
            ESP_LOGE(TAG, "❌ Falha ao gerar conteúdo do arquivo .ir");
        }
        
        // Libera memória
        if (ir_content) {
            free(ir_content);
            captured_output = NULL;
            captured_size = 0;
            captured_capacity = 0;
        }
        
    } else {
        ESP_LOGW(TAG, "⏱️  Timeout - nenhum sinal recebido");
    }

    rmt_rx_receive_stop_queue();  // Limpa a fila

    return received;
}

// Função para inicializar o storage do SD Card (substitui a do SPIFFS)
bool ir_manager_init_storage() {
    
    ir_path_err_t result = ir_path_init();
    
    if (result != IR_PATH_OK) {
        ESP_LOGE(TAG, "❌ Falha ao inicializar SD Card: %s", 
                 ir_path_error_to_string(result));
        return false;
    }
    
    // Obtem informações de armazenamento
    uint64_t total_bytes, used_bytes;
    result = ir_path_get_storage_info(&total_bytes, &used_bytes);
    
    if (result == IR_PATH_OK) {
        ESP_LOGI(TAG, "💾 SD Card: %.2f MB total, %.2f MB usados", 
                 total_bytes / (1024.0 * 1024.0), 
                 used_bytes / (1024.0 * 1024.0));
    } else {
        ESP_LOGW(TAG, "⚠️  Não foi possível obter informações do SD Card");
    }
    
    ESP_LOGI(TAG, "✅ SD Card inicializado com sucesso!");
    return true;
}

// Função para listar arquivos .ir salvos no SD Card
void ir_manager_list_files() {
    file_list_t file_list;
    ir_path_err_t result = ir_path_list_files(&file_list);
    
    if (result != IR_PATH_OK) {
        ESP_LOGE(TAG, "❌ Falha ao listar arquivos do SD Card: %s", 
                 ir_path_error_to_string(result));
        return;
    }
    
    ESP_LOGI(TAG, "📁 Arquivos no SD Card:");
    int ir_count = 0;
    
    for (int i = 0; i < file_list.count; i++) {
        if (strstr(file_list.files[i].name, ".ir") != NULL) {
            ESP_LOGI(TAG, "   %d. %s (%zu bytes)", 
                     ++ir_count, 
                     file_list.files[i].name, 
                     file_list.files[i].size);
        }
    }
    
    if (ir_count == 0) {
        ESP_LOGI(TAG, "   (nenhum arquivo .ir encontrado)");
    }
    
    ESP_LOGI(TAG, "📊 Total de arquivos: %d | Arquivos .ir: %d", 
             file_list.count, ir_count);
}

// Função adicional para ler um arquivo .ir específico do SD Card
bool ir_manager_read_file(const char* filename, char* buffer, size_t buffer_size) {
    if (!filename || !buffer || buffer_size == 0) {
        ESP_LOGE(TAG, "❌ Parâmetros inválidos para leitura de arquivo");
        return false;
    }
    
    ir_path_err_t result = ir_path_read_file(filename, buffer, buffer_size);
    
    if (result != IR_PATH_OK) {
        ESP_LOGE(TAG, "❌ Falha ao ler arquivo %s: %s", 
                 filename, ir_path_error_to_string(result));
        return false;
    }
    
    ESP_LOGI(TAG, "✅ Arquivo %s lido com sucesso", filename);
    return true;
}

// Função adicional para deletar um arquivo .ir do SD Card
bool ir_manager_delete_file(const char* filename) {
    if (!filename) {
        ESP_LOGE(TAG, "❌ Nome do arquivo inválido");
        return false;
    }
    
    // Verifica se o arquivo existe primeiro
    if (!ir_path_file_exists(filename)) {
        ESP_LOGW(TAG, "⚠️  Arquivo não encontrado: %s", filename);
        return false;
    }
    
    ir_path_err_t result = ir_path_delete_file(filename);
    
    if (result != IR_PATH_OK) {
        ESP_LOGE(TAG, "❌ Falha ao deletar arquivo %s: %s", 
                 filename, ir_path_error_to_string(result));
        return false;
    }
    
    ESP_LOGI(TAG, "✅ Arquivo %s deletado com sucesso", filename);
    return true;
}

// Função adicional para copiar um arquivo .ir
bool ir_manager_copy_file(const char* src_filename, const char* dst_filename) {
    if (!src_filename || !dst_filename) {
        ESP_LOGE(TAG, "❌ Nomes de arquivo inválidos");
        return false;
    }
    
    ir_path_err_t result = ir_path_copy_file(src_filename, dst_filename);
    
    if (result != IR_PATH_OK) {
        ESP_LOGE(TAG, "❌ Falha ao copiar arquivo %s -> %s: %s", 
                 src_filename, dst_filename, ir_path_error_to_string(result));
        return false;
    }
    
    ESP_LOGI(TAG, "✅ Arquivo copiado com sucesso: %s -> %s", 
             src_filename, dst_filename);
    return true;
}

// Função adicional para renomear um arquivo .ir
bool ir_manager_rename_file(const char* old_filename, const char* new_filename) {
    if (!old_filename || !new_filename) {
        ESP_LOGE(TAG, "❌ Nomes de arquivo inválidos");
        return false;
    }
    
    ir_path_err_t result = ir_path_rename_file(old_filename, new_filename);
    
    if (result != IR_PATH_OK) {
        ESP_LOGE(TAG, "❌ Falha ao renomear arquivo %s -> %s: %s", 
                 old_filename, new_filename, ir_path_error_to_string(result));
        return false;
    }
    
    ESP_LOGI(TAG, "✅ Arquivo renomeado com sucesso: %s -> %s", 
             old_filename, new_filename);
    return true;
}

// Função para finalizar o SD Card (cleanup)
void ir_manager_deinit_storage() {
    ir_path_err_t result = ir_path_deinit();
    
    if (result != IR_PATH_OK) {
        ESP_LOGW(TAG, "⚠️  Aviso ao finalizar SD Card: %s", 
                 ir_path_error_to_string(result));
    } else {
        ESP_LOGI(TAG, "✅ SD Card finalizado com sucesso");
    }
}

// Função para gerar o conteúdo do arquivo .ir (movida da main)
char* generate_ir_file_content(const char* signal_name, const ir_decoded_data_t* decoded) {
    static char buffer[1024];
    
    snprintf(buffer, sizeof(buffer),
        "Filetype: IR signals file\n"
        "Version: 1\n"
        "# Generated by HIGHBOY\n"
        "# \n"
        "name: %s\n"
        "type: parsed\n"
        "protocol: %s\n"
        "address: %02lX %02lX %02lX %02lX\n"
        "command: %02lX %02lX %02lX %02lX\n",
        signal_name,
        decoded->protocol,
        // Endereço formatado em 4 bytes
        (unsigned long)(decoded->address & 0xFF),
        (unsigned long)((decoded->address >> 8) & 0xFF),
        (unsigned long)((decoded->address >> 16) & 0xFF),
        (unsigned long)((decoded->address >> 24) & 0xFF),
        // Comando formatado em 4 bytes
        (unsigned long)(decoded->command & 0xFF),
        (unsigned long)((decoded->command >> 8) & 0xFF),
        (unsigned long)((decoded->command >> 16) & 0xFF),
        (unsigned long)((decoded->command >> 24) & 0xFF)
    );
    
    return buffer;
}

// Função para decodificar Sony SIRC (movida da main)
uint32_t decode_sony_sirc(rmt_symbol_word_t* symbols, size_t count) {
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

// Função para decodificar RC6 (movida da main)
uint32_t decode_rc6(rmt_symbol_word_t* symbols, size_t count) {
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

// Função para decodificar dados do protocolo (movida da main)
void decode_protocol_data(const char* protocol, rmt_symbol_word_t* symbols, size_t count, ir_decoded_data_t* decoded) {
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

// Função para gerar nome do arquivo baseado no timestamp (movida da main)
void generate_filename_with_timestamp(char* filename, size_t size, const char* protocol) {
    // Usa tick count como timestamp simples
    uint32_t timestamp = xTaskGetTickCount();
    snprintf(filename, size, "%s_%lu.ir", protocol, (unsigned long)timestamp);
}

// Função principal encapsulada
void ir_manager_receive_ir() {
    ESP_LOGI(TAG, "🚀 Iniciando captura de sinal IR...");

    // Inicializa o módulo de storage (SD Card)
    if (!ir_manager_init_storage()) {
        ESP_LOGE(TAG, "❌ Falha ao inicializar storage");
        return;
    }

    // Verifica se o manager está inicializado
    if (!has_manager_init) {
        ESP_LOGE(TAG, "❌ IR MANAGER NÃO INICIALIZADO");
        ir_manager_deinit_storage();
        return;
    }

    // Inicia a fila de recepção
    if (!rmt_rx_receive_start_queue()) {
        ESP_LOGE(TAG, "❌ Falha ao criar fila de recepção");
        ir_manager_deinit_storage();
        return;
    }

    // Buffer para armazenar símbolos recebidos
    rmt_symbol_word_t raw_symbols[MEM_BLOCK_SYMBOLS];
    size_t received = 0;

    // Tenta receber um sinal IR
    if (rmt_rx_receive_once(raw_symbols, MEM_BLOCK_SYMBOLS, &received)) {
        ESP_LOGI(TAG, "📡 Recebido %d símbolos IR", (int)received);
        
        // DETECÇÃO DO PROTOCOLO
        const char* protocol = detect_ir_protocol(raw_symbols, received);
        ESP_LOGI(TAG, "🎯 PROTOCOLO DETECTADO: %s", protocol);
        
        // ANÁLISE DETALHADA
        analyze_your_signal(raw_symbols, received);
        
        // DECODIFICAÇÃO DOS DADOS
        ir_decoded_data_t decoded_data;
        decode_protocol_data(protocol, raw_symbols, received, &decoded_data);
        
        // Mostra primeiros símbolos para debug
        ESP_LOGI(TAG, "📊 Primeiros símbolos recebidos:");
        for (size_t i = 0; i < received && i < 10; i++) {
            ESP_LOGI(TAG, "  Simb[%d] L0=%d D0=%dus L1=%d D1=%dus", (int)i,
                     raw_symbols[i].level0, raw_symbols[i].duration0,
                     raw_symbols[i].level1, raw_symbols[i].duration1);
        }
        
        // GERAÇÃO DO NOME DO ARQUIVO
        char filename[128];
        generate_ir_filename(filename, sizeof(filename), protocol);
        
        // Gera nome do sinal baseado no protocolo e dados
        char signal_name[64];
        snprintf(signal_name, sizeof(signal_name), "%s_Signal_%04X_%04X", 
                 protocol, 
                 (unsigned int)decoded_data.address, 
                 (unsigned int)decoded_data.command);
        
        // MÉTODO 1: Usa a função da main para gerar conteúdo
        char* file_content = generate_ir_file_content(signal_name, &decoded_data);
        
        // SALVA O ARQUIVO NO SD CARD
        ESP_LOGI(TAG, "💾 Salvando arquivo: %s", filename);
        ir_path_err_t save_result = ir_path_write_file(filename, file_content, false);
        
        if (save_result == IR_PATH_OK) {
            ESP_LOGI(TAG, "✅ Arquivo .ir salvo com sucesso!");
            ESP_LOGI(TAG, "📄 Nome do arquivo: %s", filename);
            ESP_LOGI(TAG, "📄 Nome do sinal: %s", signal_name);
            ESP_LOGI(TAG, "📄 Protocolo: %s", decoded_data.protocol);
            ESP_LOGI(TAG, "📄 Endereço: 0x%08X", (unsigned int)decoded_data.address);
            ESP_LOGI(TAG, "📄 Comando: 0x%08X", (unsigned int)decoded_data.command);
            
            // VERIFICAÇÃO: Lê o arquivo salvo para confirmar
            ESP_LOGI(TAG, "🔍 Verificando arquivo salvo...");
            char read_buffer[1024];
            
            if (ir_path_read_file(filename, read_buffer, sizeof(read_buffer)) == IR_PATH_OK) {
                ESP_LOGI(TAG, "✅ Arquivo lido com sucesso!");
                ESP_LOGI(TAG, "📖 === CONTEÚDO LIDO DO ARQUIVO ===");
                printf("%s", read_buffer);
                ESP_LOGI(TAG, "📖 === FIM DO ARQUIVO LIDO ===");
                
                // Verifica se o conteúdo lido é igual ao escrito
                if (strcmp(file_content, read_buffer) == 0) {
                    ESP_LOGI(TAG, "✅ Verificação OK: Conteúdo escrito = Conteúdo lido");
                } else {
                    ESP_LOGW(TAG, "⚠️ Diferença detectada entre escrito e lido!");
                    ESP_LOGW(TAG, "   Tamanho escrito: %d, Tamanho lido: %d", 
                             strlen(file_content), strlen(read_buffer));
                }
            } else {
                ESP_LOGE(TAG, "❌ Erro ao ler arquivo para verificação");
            }
            
            // MÉTODO 2: Também gera usando ir_file_auto_print para comparação
            ESP_LOGI(TAG, "🔄 Gerando arquivo alternativo usando ir_file_auto_print...");
            
            // Inicia captura do output
            start_output_capture();
            
            // Temporariamente substitui printf para capturar saída
            #define printf printf_capture
            
            // Gera arquivo usando a função existente
            ir_file_auto_print(signal_name, raw_symbols, received);
            
            // Restaura printf original
            #undef printf
            
            // Obtém conteúdo capturado
            char* ir_content_alt = finish_output_capture();
            
            if (ir_content_alt && strlen(ir_content_alt) > 0) {
                // Salva arquivo alternativo
                char filename_alt[128];
                //snprintf(filename_alt, sizeof(filename_alt), "ALT_%s", filename);
                
                if (ir_path_write_file(filename_alt, ir_content_alt, false) == IR_PATH_OK) {
                    ESP_LOGI(TAG, "✅ Arquivo alternativo salvo: %s", filename_alt);
                } else {
                    ESP_LOGW(TAG, "⚠️ Falha ao salvar arquivo alternativo");
                }
                
                // Libera memória
                free(ir_content_alt);
            }
            
        } else {
            ESP_LOGE(TAG, "❌ Erro ao salvar arquivo .ir: %s", 
                     ir_path_error_to_string(save_result));
        }
        
        // Lista arquivos após salvar
        ESP_LOGI(TAG, "📁 Listando arquivos no SD Card:");
        ir_manager_list_files();
        
    } else {
        ESP_LOGW(TAG, "⏱️ Timeout - nenhum sinal recebido");
    }

    // Cleanup
    rmt_rx_receive_stop_queue();
    ir_manager_deinit_storage();
    
    ESP_LOGI(TAG, "🏁 Captura de sinal IR finalizada.");
}

/* size_t ir_manager_receive(rmt_symbol_word_t *raw_symbols){

    size_t received = 0;

    if(!has_manager_init){
        ESP_LOGE(TAG, "IR MANAGER NÃO INICIALIZADO");
        return received;
    }

    if (!rmt_rx_receive_start_queue()) {
        ESP_LOGE(TAG, "FALHA AO CRIAR FILA DE RECEPÇÃO");
        return received;
    }

    if (rmt_rx_receive_once(raw_symbols, MEM_BLOCK_SYMBOLS, &received)) {
        ESP_LOGI(TAG, "Recebido %d símbolos IR", (int)received);
        for (size_t i = 0; i < received && i < 10; i++) {
            ESP_LOGI(TAG, "Simb[%d] L0=%d D0=%dus L1=%d D1=%dus", (int)i,
                     raw_symbols[i].level0, raw_symbols[i].duration0,
                     raw_symbols[i].level1, raw_symbols[i].duration1);
        }
    } else {
        ESP_LOGW(TAG, "Timeout - nenhum sinal recebido");
    }

    rmt_rx_receive_stop_queue();  // Limpa a fila

    return received;

} */

// - RECEIVE

bool ir_manager_transmit_raw(rmt_symbol_word_t *raw_symbols, size_t num_symbols){

    if(!has_manager_init){
        ESP_LOGE(TAG, "IR MANAGER NÃO INICIALIZADO");
        return false;
    }

    rmt_encoder_handle_t copy_encoder = NULL;
    rmt_copy_encoder_config_t copy_encoder_config = {};
    
    esp_err_t err_encoder = rmt_new_copy_encoder(&copy_encoder_config, &copy_encoder);

    if (err_encoder != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao criar encoder: %s", esp_err_to_name(err_encoder));
        return false;
    }

    ESP_LOGI(TAG, "Copy encoder criado com sucesso.");
    
    ESP_LOGI("RMT", "Iniciando transmissão...");
    

    if(!rmt_tx_transmit(copy_encoder, raw_symbols, num_symbols)){
        return false;
    }
    return true;
}

//IMPROVE
/* bool ir_manager_transmit_by_file_name(const char* signal_name) {
    
    if (!has_manager_init) {
        ESP_LOGE(TAG, "IR MANAGER NÃO INICIALIZADO");
        return false;
    }

    if (signal_name == NULL) {
        ESP_LOGE(TAG, "Nome do sinal não pode ser NULL");
        return false;
    }

    // Ler arquivo IR
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/spiffs/%s.ir", signal_name);
    
    FILE* file = fopen(filepath, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Não foi possível abrir arquivo: %s", filepath);
        return false;
    }

    // Ler conteúdo do arquivo
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* file_content = malloc(file_size + 1);
    if (file_content == NULL) {
        ESP_LOGE(TAG, "Erro ao alocar memória para arquivo");
        fclose(file);
        return false;
    }

    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0';
    fclose(file);

    // Criar encoder IR
    ir_encoder_handle_t ir_encoder = NULL;
    ir_encoder_config_t ir_config = IR_ENCODER_DEFAULT_CONFIG();
    esp_err_t err = ir_encoder_new(&ir_config, &ir_encoder);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao criar IR encoder: %s", esp_err_to_name(err));
        free(file_content);
        return false;
    }

    // Parse do arquivo IR
    ir_file_data_t file_data;
    bool parse_success = ir_file_parse_string(file_content, &file_data);
    
    if (!parse_success) {
        ESP_LOGE(TAG, "Falha ao fazer parse do arquivo: %s", filepath);
        ir_encoder_del(ir_encoder);
        free(file_content);
        return false;
    }

    // Transmitir o sinal
    bool transmission_success = ir_file_transmit(&file_data, ir_encoder);
    
    if (transmission_success) {
        ESP_LOGI(TAG, "✅ Arquivo '%s' transmitido com sucesso!", signal_name);
    } else {
        ESP_LOGE(TAG, "❌ Falha ao transmitir arquivo '%s'", signal_name);
    }

    // Cleanup
    ir_file_free_data(&file_data);
    ir_encoder_del(ir_encoder);
    free(file_content);

    return transmission_success;
}
 */

bool ir_manager_transmit_by_file_name(const char* signal_name) {
    
    if (!has_manager_init) {
        ESP_LOGE(TAG, "IR MANAGER NÃO INICIALIZADO");
        return false;
    }

    if (signal_name == NULL) {
        ESP_LOGE(TAG, "Nome do sinal não pode ser NULL");
        return false;
    }

    // Construir nome do arquivo com extensão .ir
    char filename[MAX_FILENAME_LEN];
    int ret = snprintf(filename, sizeof(filename), "%s.ir", signal_name);
    if (ret >= sizeof(filename) || ret < 0) {
        ESP_LOGE(TAG, "Nome do arquivo muito longo: %s", signal_name);
        return false;
    }

    // Verificar se o arquivo existe
    if (!ir_path_file_exists(filename)) {
        ESP_LOGE(TAG, "Arquivo não encontrado: %s", filename);
        return false;
    }

    // Obter tamanho do arquivo
    size_t file_size;
    ir_path_err_t path_err = ir_path_get_file_size(filename, &file_size);
    if (path_err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Erro ao obter tamanho do arquivo: %s", ir_path_error_to_string(path_err));
        return false;
    }

    // Alocar buffer para o conteúdo do arquivo
    char* file_content = malloc(file_size + 1);
    if (file_content == NULL) {
        ESP_LOGE(TAG, "Erro ao alocar memória para arquivo (%zu bytes)", file_size);
        return false;
    }

    // Ler conteúdo do arquivo usando ir_path
    path_err = ir_path_read_file(filename, file_content, file_size + 1);
    if (path_err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Erro ao ler arquivo: %s", ir_path_error_to_string(path_err));
        free(file_content);
        return false;
    }

    // Criar encoder IR usando a biblioteca ir_encoder
    ir_encoder_handle_t ir_encoder = NULL;
    ir_encoder_config_t ir_config = IR_ENCODER_DEFAULT_CONFIG();
    esp_err_t err = ir_encoder_new(&ir_config, &ir_encoder);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao criar IR encoder: %s", esp_err_to_name(err));
        free(file_content);
        return false;
    }

    // Parse do arquivo IR
    ir_file_data_t file_data;
    bool parse_success = ir_file_parse_string(file_content, &file_data);
    
    if (!parse_success) {
        ESP_LOGE(TAG, "Falha ao fazer parse do arquivo: %s", filename);
        ir_encoder_del(ir_encoder);
        free(file_content);
        return false;
    }

    // Log de informações do arquivo antes da transmissão
    ESP_LOGI(TAG, "Transmitindo arquivo: %s", filename);
    ESP_LOGI(TAG, "Protocolo: %s, Frequência: %"PRIu32"Hz", 
         file_data.protocol, file_data.frequency);

    // Transmitir o sinal
    bool transmission_success = ir_file_transmit(&file_data, ir_encoder);
    
    if (transmission_success) {
        ESP_LOGI(TAG, "✅ Arquivo '%s' transmitido com sucesso!", signal_name);
    } else {
        ESP_LOGE(TAG, "❌ Falha ao transmitir arquivo '%s'", signal_name);
    }

    // Cleanup
    ir_file_free_data(&file_data);
    ir_encoder_del(ir_encoder);
    free(file_content);

    return transmission_success;
}

// Função auxiliar para listar arquivos IR disponíveis
bool ir_manager_list_ir_files(void) {
    if (!has_manager_init) {
        ESP_LOGE(TAG, "IR MANAGER NÃO INICIALIZADO");
        return false;
    }

    file_list_t file_list;
    ir_path_err_t err = ir_path_list_files(&file_list);
    
    if (err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Erro ao listar arquivos: %s", ir_path_error_to_string(err));
        return false;
    }

    ESP_LOGI(TAG, "Arquivos IR disponíveis (%d):", file_list.count);
    for (int i = 0; i < file_list.count; i++) {
        if (!file_list.files[i].is_directory) {
            // Verifica se é arquivo .ir
            char* ext = strrchr(file_list.files[i].name, '.');
            if (ext && strcmp(ext, ".ir") == 0) {
                ESP_LOGI(TAG, "  - %s (%zu bytes)", 
                         file_list.files[i].name, file_list.files[i].size);
            }
        }
    }

    return true;
}

// Função para verificar se um arquivo IR existe
bool ir_manager_signal_exists(const char* signal_name) {
    if (!has_manager_init || !signal_name) {
        return false;
    }

    char filename[MAX_FILENAME_LEN];
    int ret = snprintf(filename, sizeof(filename), "%s.ir", signal_name);
    if (ret >= sizeof(filename) || ret < 0) {
        return false;
    }

    return ir_path_file_exists(filename);
}

// Função para deletar um arquivo IR
bool ir_manager_delete_signal(const char* signal_name) {
    if (!has_manager_init) {
        ESP_LOGE(TAG, "IR MANAGER NÃO INICIALIZADO");
        return false;
    }

    if (signal_name == NULL) {
        ESP_LOGE(TAG, "Nome do sinal não pode ser NULL");
        return false;
    }

    char filename[MAX_FILENAME_LEN];
    int ret = snprintf(filename, sizeof(filename), "%s.ir", signal_name);
    if (ret >= sizeof(filename) || ret < 0) {
        ESP_LOGE(TAG, "Nome do arquivo muito longo: %s", signal_name);
        return false;
    }

    ir_path_err_t err = ir_path_delete_file(filename);
    if (err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Erro ao deletar arquivo: %s", ir_path_error_to_string(err));
        return false;
    }

    ESP_LOGI(TAG, "Arquivo '%s' deletado com sucesso", signal_name);
    return true;
}

// Função para copiar um arquivo IR
bool ir_manager_copy_signal(const char* src_signal_name, const char* dst_signal_name) {
    if (!has_manager_init) {
        ESP_LOGE(TAG, "IR MANAGER NÃO INICIALIZADO");
        return false;
    }

    if (!src_signal_name || !dst_signal_name) {
        ESP_LOGE(TAG, "Nomes dos sinais não podem ser NULL");
        return false;
    }

    char src_filename[MAX_FILENAME_LEN];
    char dst_filename[MAX_FILENAME_LEN];
    
    int ret1 = snprintf(src_filename, sizeof(src_filename), "%s.ir", src_signal_name);
    int ret2 = snprintf(dst_filename, sizeof(dst_filename), "%s.ir", dst_signal_name);
    
    if (ret1 >= sizeof(src_filename) || ret1 < 0 || 
        ret2 >= sizeof(dst_filename) || ret2 < 0) {
        ESP_LOGE(TAG, "Nome de arquivo muito longo");
        return false;
    }

    ir_path_err_t err = ir_path_copy_file(src_filename, dst_filename);
    if (err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Erro ao copiar arquivo: %s", ir_path_error_to_string(err));
        return false;
    }

    ESP_LOGI(TAG, "Arquivo copiado: '%s' -> '%s'", src_signal_name, dst_signal_name);
    return true;
}

// Função para renomear um arquivo IR
bool ir_manager_rename_signal(const char* old_signal_name, const char* new_signal_name) {
    if (!has_manager_init) {
        ESP_LOGE(TAG, "IR MANAGER NÃO INICIALIZADO");
        return false;
    }

    if (!old_signal_name || !new_signal_name) {
        ESP_LOGE(TAG, "Nomes dos sinais não podem ser NULL");
        return false;
    }

    char old_filename[MAX_FILENAME_LEN];
    char new_filename[MAX_FILENAME_LEN];
    
    int ret1 = snprintf(old_filename, sizeof(old_filename), "%s.ir", old_signal_name);
    int ret2 = snprintf(new_filename, sizeof(new_filename), "%s.ir", new_signal_name);
    
    if (ret1 >= sizeof(old_filename) || ret1 < 0 || 
        ret2 >= sizeof(new_filename) || ret2 < 0) {
        ESP_LOGE(TAG, "Nome de arquivo muito longo");
        return false;
    }

    ir_path_err_t err = ir_path_rename_file(old_filename, new_filename);
    if (err != IR_PATH_OK) {
        ESP_LOGE(TAG, "Erro ao renomear arquivo: %s", ir_path_error_to_string(err));
        return false;
    }

    ESP_LOGI(TAG, "Arquivo renomeado: '%s' -> '%s'", old_signal_name, new_signal_name);
    return true;
}

 //**TO DO**
void ir_manager_send_brute_force(){

}

//**TO DO **
void ir_manager_send_tvbgone(){
    
}