#ifndef IR_ENCODER_H
#define IR_ENCODER_H

#include "driver/rmt_tx.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Tipos de protocolos suportados
typedef enum {
    IR_PROTOCOL_NEC,
    IR_PROTOCOL_NEC_EXT,        // NEC com endereço 16-bit
    IR_PROTOCOL_SONY,           // Sony SIRC
    IR_PROTOCOL_RC5,            // Philips RC5
    IR_PROTOCOL_RC6,            // Philips RC6
    IR_PROTOCOL_SAMSUNG,        // Samsung
    IR_PROTOCOL_PANASONIC,      // Panasonic
    IR_PROTOCOL_JVC,            // JVC
    IR_PROTOCOL_LG,             // LG
    IR_PROTOCOL_MITSUBISHI,     // Mitsubishi
    IR_PROTOCOL_SHARP,          // Sharp
    IR_PROTOCOL_DISH,           // Dish Network
    IR_PROTOCOL_AIWA,           // Aiwa RC-T501
    IR_PROTOCOL_DENON,          // Denon
    IR_PROTOCOL_KASEIKYO,       // Kaseikyo (Panasonic variant)
    IR_PROTOCOL_WHYNTER,        // Whynter AC
    IR_PROTOCOL_COOLIX,         // Coolix AC
    IR_PROTOCOL_GENERIC,        // Generic/Unknown
    IR_PROTOCOL_MAX
} ir_protocol_t;

// Estrutura para dados de comando IR
typedef struct {
    ir_protocol_t protocol;
    uint32_t address;
    uint32_t command;
    bool repeat;            // Para comandos repetidos
    uint8_t bits;           // Número de bits (para protocolos variáveis)
    uint32_t frequency;     // Frequência da portadora (Hz)
} ir_command_t;

// Handle do encoder IR
typedef struct ir_encoder_t* ir_encoder_handle_t;

// Configuração do encoder IR
typedef struct {
    uint32_t resolution;        // Resolução em Hz (padrão: 1MHz)
    uint32_t carrier_freq;      // Frequência da portadora (padrão: 38kHz)
    uint8_t duty_cycle;         // Ciclo de trabalho da portadora (padrão: 33%)
} ir_encoder_config_t;

// Configuração padrão
#define IR_ENCODER_DEFAULT_CONFIG() {           \
    .resolution = 1000000,      /* 1MHz */      \
    .carrier_freq = 38000,      /* 38kHz */     \
    .duty_cycle = 33,           /* 33% */       \
}

/**
 * @brief Criar um novo encoder IR
 * 
 * @param config Configuração do encoder
 * @param ret_encoder Handle do encoder criado
 * @return esp_err_t ESP_OK em caso de sucesso
 */
esp_err_t ir_encoder_new(const ir_encoder_config_t *config, ir_encoder_handle_t *ret_encoder);

/**
 * @brief Deletar encoder IR
 * 
 * @param encoder Handle do encoder
 * @return esp_err_t ESP_OK em caso de sucesso
 */
esp_err_t ir_encoder_del(ir_encoder_handle_t encoder);

/**
 * @brief Codificar comando IR em símbolos RMT
 * 
 * @param encoder Handle do encoder
 * @param command Comando IR para codificar
 * @param symbols Buffer para armazenar símbolos
 * @param symbols_size Tamanho do buffer de símbolos
 * @param ret_num_symbols Número de símbolos gerados
 * @return esp_err_t ESP_OK em caso de sucesso
 */
esp_err_t ir_encoder_encode(ir_encoder_handle_t encoder, 
                           const ir_command_t *command,
                           rmt_symbol_word_t *symbols,
                           size_t symbols_size,
                           size_t *ret_num_symbols);

/**
 * @brief Funções de conveniência para protocolos específicos
 */

// NEC Protocol
esp_err_t ir_encode_nec(ir_encoder_handle_t encoder, uint8_t address, uint8_t command, 
                       rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

esp_err_t ir_encode_nec_ext(ir_encoder_handle_t encoder, uint16_t address, uint8_t command,
                           rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

esp_err_t ir_encode_nec_repeat(ir_encoder_handle_t encoder,
                              rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

// Sony Protocol (SIRC)
esp_err_t ir_encode_sony(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                        rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

// RC5 Protocol
esp_err_t ir_encode_rc5(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                       rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

// RC6 Protocol
esp_err_t ir_encode_rc6(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                       rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

// Samsung Protocol
esp_err_t ir_encode_samsung(ir_encoder_handle_t encoder, uint16_t address, uint8_t command,
                           rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

// Panasonic Protocol
esp_err_t ir_encode_panasonic(ir_encoder_handle_t encoder, uint16_t address, uint32_t command,
                             rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

// JVC Protocol
esp_err_t ir_encode_jvc(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                       rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

// LG Protocol
esp_err_t ir_encode_lg(ir_encoder_handle_t encoder, uint8_t address, uint16_t command,
                      rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

// Mitsubishi Protocol
esp_err_t ir_encode_mitsubishi(ir_encoder_handle_t encoder, uint16_t address, uint16_t command,
                              rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

// Sharp Protocol
esp_err_t ir_encode_sharp(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                         rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

// Dish Network Protocol
esp_err_t ir_encode_dish(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                        rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

// Aiwa Protocol
esp_err_t ir_encode_aiwa(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                        rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

// Denon Protocol
esp_err_t ir_encode_denon(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                         rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

// Kaseikyo Protocol
esp_err_t ir_encode_kaseikyo(ir_encoder_handle_t encoder, uint16_t address, uint32_t command,
                            rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

// Whynter Protocol
esp_err_t ir_encode_whynter(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                           rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

// Coolix Protocol
esp_err_t ir_encode_coolix(ir_encoder_handle_t encoder, uint32_t command,
                          rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols);

/**
 * @brief Funções auxiliares
 */

// Converter string de protocolo para enum
ir_protocol_t ir_protocol_from_string(const char *protocol_str);

// Converter enum de protocolo para string
const char* ir_protocol_to_string(ir_protocol_t protocol);

// Obter informações sobre um protocolo
typedef struct {
    const char* name;
    uint32_t default_frequency;
    uint8_t address_bits;
    uint8_t command_bits;
    bool has_repeat;
    const char* description;
} ir_protocol_info_t;

const ir_protocol_info_t* ir_get_protocol_info(ir_protocol_t protocol);

// Listar todos os protocolos suportados
void ir_list_supported_protocols(void);

#ifdef __cplusplus
}
#endif

#endif // IR_ENCODER_H