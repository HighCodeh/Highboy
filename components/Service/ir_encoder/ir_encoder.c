#include "ir_encoder.h"
#include "esp_log.h"
#include "esp_check.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "ir_encoder";

// Estrutura interna do encoder
struct ir_encoder_t {
    uint32_t resolution;
    uint32_t carrier_freq;
    uint8_t duty_cycle;
    uint32_t tick_duration;  // Duração de um tick em microssegundos
};

// Converter microssegundos para ticks
#define US_TO_TICKS(encoder, us) ((us) * (encoder)->resolution / 1000000)

// Definições de timing para diferentes protocolos (em microssegundos)

// NEC Protocol timings
#define NEC_HEADER_HIGH_US      9000
#define NEC_HEADER_LOW_US       4500
#define NEC_BIT_ONE_HIGH_US     560
#define NEC_BIT_ONE_LOW_US      1690
#define NEC_BIT_ZERO_HIGH_US    560
#define NEC_BIT_ZERO_LOW_US     560
#define NEC_STOP_BIT_US         560
#define NEC_REPEAT_HIGH_US      9000
#define NEC_REPEAT_LOW_US       2250

// Sony Protocol timings (SIRC-12)
#define SONY_HEADER_HIGH_US     2400
#define SONY_HEADER_LOW_US      600
#define SONY_BIT_ONE_HIGH_US    1200
#define SONY_BIT_ONE_LOW_US     600
#define SONY_BIT_ZERO_HIGH_US   600
#define SONY_BIT_ZERO_LOW_US    600

// RC5 Protocol timings
#define RC5_BIT_DURATION_US     889
#define RC5_HALF_BIT_US         444

// RC6 Protocol timings
#define RC6_HEADER_HIGH_US      2666
#define RC6_HEADER_LOW_US       889
#define RC6_BIT_DURATION_US     444
#define RC6_HALF_BIT_US         222

// Samsung Protocol timings
#define SAMSUNG_HEADER_HIGH_US  4500
#define SAMSUNG_HEADER_LOW_US   4500
#define SAMSUNG_BIT_ONE_HIGH_US 560
#define SAMSUNG_BIT_ONE_LOW_US  1690
#define SAMSUNG_BIT_ZERO_HIGH_US 560
#define SAMSUNG_BIT_ZERO_LOW_US 560
#define SAMSUNG_STOP_BIT_US     560

// Panasonic Protocol timings
#define PANASONIC_HEADER_HIGH_US 3502
#define PANASONIC_HEADER_LOW_US  1750
#define PANASONIC_BIT_ONE_HIGH_US 502
#define PANASONIC_BIT_ONE_LOW_US  1244
#define PANASONIC_BIT_ZERO_HIGH_US 502
#define PANASONIC_BIT_ZERO_LOW_US 400

// JVC Protocol timings
#define JVC_HEADER_HIGH_US      8400
#define JVC_HEADER_LOW_US       4200
#define JVC_BIT_ONE_HIGH_US     600
#define JVC_BIT_ONE_LOW_US      1600
#define JVC_BIT_ZERO_HIGH_US    600
#define JVC_BIT_ZERO_LOW_US     550

// LG Protocol timings
#define LG_HEADER_HIGH_US       9000
#define LG_HEADER_LOW_US        4500
#define LG_BIT_ONE_HIGH_US      600
#define LG_BIT_ONE_LOW_US       1600
#define LG_BIT_ZERO_HIGH_US     600
#define LG_BIT_ZERO_LOW_US      550

// Mitsubishi Protocol timings
#define MITSUBISHI_HEADER_HIGH_US 3400
#define MITSUBISHI_HEADER_LOW_US  1750
#define MITSUBISHI_BIT_ONE_HIGH_US 450
#define MITSUBISHI_BIT_ONE_LOW_US  1300
#define MITSUBISHI_BIT_ZERO_HIGH_US 450
#define MITSUBISHI_BIT_ZERO_LOW_US 420

// Sharp Protocol timings
#define SHARP_BIT_ONE_HIGH_US   320
#define SHARP_BIT_ONE_LOW_US    1680
#define SHARP_BIT_ZERO_HIGH_US  320
#define SHARP_BIT_ZERO_LOW_US   680

// Dish Network Protocol timings
#define DISH_HEADER_HIGH_US     400
#define DISH_HEADER_LOW_US      6000
#define DISH_BIT_ONE_HIGH_US    400
#define DISH_BIT_ONE_LOW_US     2800
#define DISH_BIT_ZERO_HIGH_US   400
#define DISH_BIT_ZERO_LOW_US    1400

// Aiwa Protocol timings
#define AIWA_HEADER_HIGH_US     8800
#define AIWA_HEADER_LOW_US      4500
#define AIWA_BIT_ONE_HIGH_US    550
#define AIWA_BIT_ONE_LOW_US     1650
#define AIWA_BIT_ZERO_HIGH_US   550
#define AIWA_BIT_ZERO_LOW_US    550

// Denon Protocol timings
#define DENON_HEADER_HIGH_US    3500
#define DENON_HEADER_LOW_US     1750
#define DENON_BIT_ONE_HIGH_US   350
#define DENON_BIT_ONE_LOW_US    1300
#define DENON_BIT_ZERO_HIGH_US  350
#define DENON_BIT_ZERO_LOW_US   350

// Kaseikyo Protocol timings
#define KASEIKYO_HEADER_HIGH_US 3380
#define KASEIKYO_HEADER_LOW_US  1690
#define KASEIKYO_BIT_ONE_HIGH_US 423
#define KASEIKYO_BIT_ONE_LOW_US  1269
#define KASEIKYO_BIT_ZERO_HIGH_US 423
#define KASEIKYO_BIT_ZERO_LOW_US 423

// Whynter Protocol timings
#define WHYNTER_HEADER_HIGH_US  2850
#define WHYNTER_HEADER_LOW_US   2850
#define WHYNTER_BIT_ONE_HIGH_US 750
#define WHYNTER_BIT_ONE_LOW_US  750
#define WHYNTER_BIT_ZERO_HIGH_US 750
#define WHYNTER_BIT_ZERO_LOW_US 750

// Coolix Protocol timings
#define COOLIX_HEADER_HIGH_US   4692
#define COOLIX_HEADER_LOW_US    4416
#define COOLIX_BIT_ONE_HIGH_US  552
#define COOLIX_BIT_ONE_LOW_US   1656
#define COOLIX_BIT_ZERO_HIGH_US 552
#define COOLIX_BIT_ZERO_LOW_US  552

// Tabela de informações dos protocolos
static const ir_protocol_info_t protocol_info_table[] = {
    [IR_PROTOCOL_NEC] = {
        .name = "NEC",
        .default_frequency = 38000,
        .address_bits = 8,
        .command_bits = 8,
        .has_repeat = true,
        .description = "NEC Infrared Transmission Protocol"
    },
    [IR_PROTOCOL_NEC_EXT] = {
        .name = "NEC Extended",
        .default_frequency = 38000,
        .address_bits = 16,
        .command_bits = 8,
        .has_repeat = true,
        .description = "NEC Extended with 16-bit address"
    },
    [IR_PROTOCOL_SONY] = {
        .name = "Sony SIRC",
        .default_frequency = 40000,
        .address_bits = 5,
        .command_bits = 7,
        .has_repeat = false,
        .description = "Sony Infrared Remote Control (SIRC)"
    },
    [IR_PROTOCOL_RC5] = {
        .name = "RC5",
        .default_frequency = 36000,
        .address_bits = 5,
        .command_bits = 6,
        .has_repeat = false,
        .description = "Philips RC5 Protocol"
    },
    [IR_PROTOCOL_RC6] = {
        .name = "RC6",
        .default_frequency = 36000,
        .address_bits = 8,
        .command_bits = 8,
        .has_repeat = false,
        .description = "Philips RC6 Protocol"
    },
    [IR_PROTOCOL_SAMSUNG] = {
        .name = "Samsung",
        .default_frequency = 38000,
        .address_bits = 16,
        .command_bits = 8,
        .has_repeat = false,
        .description = "Samsung IR Protocol"
    },
    [IR_PROTOCOL_PANASONIC] = {
        .name = "Panasonic",
        .default_frequency = 36700,
        .address_bits = 16,
        .command_bits = 32,
        .has_repeat = false,
        .description = "Panasonic IR Protocol"
    },
    [IR_PROTOCOL_JVC] = {
        .name = "JVC",
        .default_frequency = 38000,
        .address_bits = 8,
        .command_bits = 8,
        .has_repeat = false,
        .description = "JVC IR Protocol"
    },
    [IR_PROTOCOL_LG] = {
        .name = "LG",
        .default_frequency = 38000,
        .address_bits = 8,
        .command_bits = 16,
        .has_repeat = false,
        .description = "LG IR Protocol"
    },
    [IR_PROTOCOL_MITSUBISHI] = {
        .name = "Mitsubishi",
        .default_frequency = 38000,
        .address_bits = 16,
        .command_bits = 16,
        .has_repeat = false,
        .description = "Mitsubishi IR Protocol"
    },
    [IR_PROTOCOL_SHARP] = {
        .name = "Sharp",
        .default_frequency = 38000,
        .address_bits = 8,
        .command_bits = 8,
        .has_repeat = false,
        .description = "Sharp IR Protocol"
    },
    [IR_PROTOCOL_DISH] = {
        .name = "Dish Network",
        .default_frequency = 57600,
        .address_bits = 8,
        .command_bits = 8,
        .has_repeat = false,
        .description = "Dish Network IR Protocol"
    },
    [IR_PROTOCOL_AIWA] = {
        .name = "Aiwa",
        .default_frequency = 38000,
        .address_bits = 8,
        .command_bits = 8,
        .has_repeat = false,
        .description = "Aiwa RC-T501 Protocol"
    },
    [IR_PROTOCOL_DENON] = {
        .name = "Denon",
        .default_frequency = 38000,
        .address_bits = 8,
        .command_bits = 8,
        .has_repeat = false,
        .description = "Denon IR Protocol"
    },
    [IR_PROTOCOL_KASEIKYO] = {
        .name = "Kaseikyo",
        .default_frequency = 36700,
        .address_bits = 16,
        .command_bits = 32,
        .has_repeat = false,
        .description = "Kaseikyo (Panasonic variant) Protocol"
    },
    [IR_PROTOCOL_WHYNTER] = {
        .name = "Whynter",
        .default_frequency = 38000,
        .address_bits = 8,
        .command_bits = 8,
        .has_repeat = false,
        .description = "Whynter Air Conditioner Protocol"
    },
    [IR_PROTOCOL_COOLIX] = {
        .name = "Coolix",
        .default_frequency = 38000,
        .address_bits = 0,
        .command_bits = 32,
        .has_repeat = false,
        .description = "Coolix Air Conditioner Protocol"
    },
    [IR_PROTOCOL_GENERIC] = {
        .name = "Generic",
        .default_frequency = 38000,
        .address_bits = 16,
        .command_bits = 16,
        .has_repeat = false,
        .description = "Generic/Unknown IR Protocol"
    }
};

esp_err_t ir_encoder_new(const ir_encoder_config_t *config, ir_encoder_handle_t *ret_encoder)
{
    ESP_RETURN_ON_FALSE(config && ret_encoder, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    
    ir_encoder_handle_t encoder = calloc(1, sizeof(struct ir_encoder_t));
    ESP_RETURN_ON_FALSE(encoder, ESP_ERR_NO_MEM, TAG, "no memory for encoder");
    
    encoder->resolution = config->resolution;
    encoder->carrier_freq = config->carrier_freq;
    encoder->duty_cycle = config->duty_cycle;
    encoder->tick_duration = 1000000 / config->resolution;
    
    *ret_encoder = encoder;
    ESP_LOGI(TAG, "IR encoder created, resolution: %"PRIu32"Hz, carrier: %"PRIu32"Hz", 
             config->resolution, config->carrier_freq);
    
    return ESP_OK;
}

esp_err_t ir_encoder_del(ir_encoder_handle_t encoder)
{
    ESP_RETURN_ON_FALSE(encoder, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    free(encoder);
    return ESP_OK;
}

// Funções auxiliares para criação de símbolos
static inline rmt_symbol_word_t make_symbol(ir_encoder_handle_t encoder, 
                                           uint32_t high_us, uint32_t low_us)
{
    rmt_symbol_word_t symbol = {
        .level0 = 1,
        .duration0 = US_TO_TICKS(encoder, high_us),
        .level1 = 0,
        .duration1 = US_TO_TICKS(encoder, low_us)
    };
    return symbol;
}

// Funções para adicionar bits de diferentes protocolos
static void add_bit_nec(ir_encoder_handle_t encoder, rmt_symbol_word_t *symbols, 
                       size_t *index, uint8_t bit)
{
    if (bit) {
        symbols[*index] = make_symbol(encoder, NEC_BIT_ONE_HIGH_US, NEC_BIT_ONE_LOW_US);
    } else {
        symbols[*index] = make_symbol(encoder, NEC_BIT_ZERO_HIGH_US, NEC_BIT_ZERO_LOW_US);
    }
    (*index)++;
}

static void add_bit_sony(ir_encoder_handle_t encoder, rmt_symbol_word_t *symbols, 
                        size_t *index, uint8_t bit)
{
    if (bit) {
        symbols[*index] = make_symbol(encoder, SONY_BIT_ONE_HIGH_US, SONY_BIT_ONE_LOW_US);
    } else {
        symbols[*index] = make_symbol(encoder, SONY_BIT_ZERO_HIGH_US, SONY_BIT_ZERO_LOW_US);
    }
    (*index)++;
}

static void add_bit_samsung(ir_encoder_handle_t encoder, rmt_symbol_word_t *symbols, 
                           size_t *index, uint8_t bit)
{
    if (bit) {
        symbols[*index] = make_symbol(encoder, SAMSUNG_BIT_ONE_HIGH_US, SAMSUNG_BIT_ONE_LOW_US);
    } else {
        symbols[*index] = make_symbol(encoder, SAMSUNG_BIT_ZERO_HIGH_US, SAMSUNG_BIT_ZERO_LOW_US);
    }
    (*index)++;
}

static void add_bit_panasonic(ir_encoder_handle_t encoder, rmt_symbol_word_t *symbols, 
                             size_t *index, uint8_t bit)
{
    if (bit) {
        symbols[*index] = make_symbol(encoder, PANASONIC_BIT_ONE_HIGH_US, PANASONIC_BIT_ONE_LOW_US);
    } else {
        symbols[*index] = make_symbol(encoder, PANASONIC_BIT_ZERO_HIGH_US, PANASONIC_BIT_ZERO_LOW_US);
    }
    (*index)++;
}

static void add_bit_jvc(ir_encoder_handle_t encoder, rmt_symbol_word_t *symbols, 
                       size_t *index, uint8_t bit)
{
    if (bit) {
        symbols[*index] = make_symbol(encoder, JVC_BIT_ONE_HIGH_US, JVC_BIT_ONE_LOW_US);
    } else {
        symbols[*index] = make_symbol(encoder, JVC_BIT_ZERO_HIGH_US, JVC_BIT_ZERO_LOW_US);
    }
    (*index)++;
}

static void add_bit_lg(ir_encoder_handle_t encoder, rmt_symbol_word_t *symbols, 
                      size_t *index, uint8_t bit)
{
    if (bit) {
        symbols[*index] = make_symbol(encoder, LG_BIT_ONE_HIGH_US, LG_BIT_ONE_LOW_US);
    } else {
        symbols[*index] = make_symbol(encoder, LG_BIT_ZERO_HIGH_US, LG_BIT_ZERO_LOW_US);
    }
    (*index)++;
}

static void add_bit_mitsubishi(ir_encoder_handle_t encoder, rmt_symbol_word_t *symbols, 
                              size_t *index, uint8_t bit)
{
    if (bit) {
        symbols[*index] = make_symbol(encoder, MITSUBISHI_BIT_ONE_HIGH_US, MITSUBISHI_BIT_ONE_LOW_US);
    } else {
        symbols[*index] = make_symbol(encoder, MITSUBISHI_BIT_ZERO_HIGH_US, MITSUBISHI_BIT_ZERO_LOW_US);
    }
    (*index)++;
}

static void add_bit_sharp(ir_encoder_handle_t encoder, rmt_symbol_word_t *symbols, 
                         size_t *index, uint8_t bit)
{
    if (bit) {
        symbols[*index] = make_symbol(encoder, SHARP_BIT_ONE_HIGH_US, SHARP_BIT_ONE_LOW_US);
    } else {
        symbols[*index] = make_symbol(encoder, SHARP_BIT_ZERO_HIGH_US, SHARP_BIT_ZERO_LOW_US);
    }
    (*index)++;
}

static void add_bit_dish(ir_encoder_handle_t encoder, rmt_symbol_word_t *symbols, 
                        size_t *index, uint8_t bit)
{
    if (bit) {
        symbols[*index] = make_symbol(encoder, DISH_BIT_ONE_HIGH_US, DISH_BIT_ONE_LOW_US);
    } else {
        symbols[*index] = make_symbol(encoder, DISH_BIT_ZERO_HIGH_US, DISH_BIT_ZERO_LOW_US);
    }
    (*index)++;
}

static void add_bit_aiwa(ir_encoder_handle_t encoder, rmt_symbol_word_t *symbols, 
                        size_t *index, uint8_t bit)
{
    if (bit) {
        symbols[*index] = make_symbol(encoder, AIWA_BIT_ONE_HIGH_US, AIWA_BIT_ONE_LOW_US);
    } else {
        symbols[*index] = make_symbol(encoder, AIWA_BIT_ZERO_HIGH_US, AIWA_BIT_ZERO_LOW_US);
    }
    (*index)++;
}

static void add_bit_denon(ir_encoder_handle_t encoder, rmt_symbol_word_t *symbols, 
                         size_t *index, uint8_t bit)
{
    if (bit) {
        symbols[*index] = make_symbol(encoder, DENON_BIT_ONE_HIGH_US, DENON_BIT_ONE_LOW_US);
    } else {
        symbols[*index] = make_symbol(encoder, DENON_BIT_ZERO_HIGH_US, DENON_BIT_ZERO_LOW_US);
    }
    (*index)++;
}

static void add_bit_kaseikyo(ir_encoder_handle_t encoder, rmt_symbol_word_t *symbols, 
                            size_t *index, uint8_t bit)
{
    if (bit) {
        symbols[*index] = make_symbol(encoder, KASEIKYO_BIT_ONE_HIGH_US, KASEIKYO_BIT_ONE_LOW_US);
    } else {
        symbols[*index] = make_symbol(encoder, KASEIKYO_BIT_ZERO_HIGH_US, KASEIKYO_BIT_ZERO_LOW_US);
    }
    (*index)++;
}

static void add_bit_whynter(ir_encoder_handle_t encoder, rmt_symbol_word_t *symbols, 
                           size_t *index, uint8_t bit)
{
    if (bit) {
        symbols[*index] = make_symbol(encoder, WHYNTER_BIT_ONE_HIGH_US, WHYNTER_BIT_ONE_LOW_US);
    } else {
        symbols[*index] = make_symbol(encoder, WHYNTER_BIT_ZERO_HIGH_US, WHYNTER_BIT_ZERO_LOW_US);
    }
    (*index)++;
}

static void add_bit_coolix(ir_encoder_handle_t encoder, rmt_symbol_word_t *symbols, 
                          size_t *index, uint8_t bit)
{
    if (bit) {
        symbols[*index] = make_symbol(encoder, COOLIX_BIT_ONE_HIGH_US, COOLIX_BIT_ONE_LOW_US);
    } else {
        symbols[*index] = make_symbol(encoder, COOLIX_BIT_ZERO_HIGH_US, COOLIX_BIT_ZERO_LOW_US);
    }
    (*index)++;
}

// Implementações dos protocolos
esp_err_t ir_encode_nec(ir_encoder_handle_t encoder, uint8_t address, uint8_t command, 
                       rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 34, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for NEC");
    
    size_t index = 0;
    
    // Header
    symbols[index++] = make_symbol(encoder, NEC_HEADER_HIGH_US, NEC_HEADER_LOW_US);
    
    // Address (8 bits LSB first)
    for (int i = 0; i < 8; i++) {
        add_bit_nec(encoder, symbols, &index, (address >> i) & 1);
    }
    
    // Address inverse (8 bits LSB first)
    uint8_t address_inv = ~address;
    for (int i = 0; i < 8; i++) {
        add_bit_nec(encoder, symbols, &index, (address_inv >> i) & 1);
    }
    
    // Command (8 bits LSB first)
    for (int i = 0; i < 8; i++) {
        add_bit_nec(encoder, symbols, &index, (command >> i) & 1);
    }
    
    // Command inverse (8 bits LSB first)
    uint8_t command_inv = ~command;
    for (int i = 0; i < 8; i++) {
        add_bit_nec(encoder, symbols, &index, (command_inv >> i) & 1);
    }
    
    // Stop bit
    symbols[index++] = make_symbol(encoder, NEC_STOP_BIT_US, 0);
    
    *ret_num_symbols = index;
    ESP_LOGD(TAG, "NEC encoded: addr=0x%02X, cmd=0x%02X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_nec_ext(ir_encoder_handle_t encoder, uint16_t address, uint8_t command,
                           rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 34, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for NEC Extended");
    
    size_t index = 0;
    
    // Header
    symbols[index++] = make_symbol(encoder, NEC_HEADER_HIGH_US, NEC_HEADER_LOW_US);
    
    // Address (16 bits LSB first)
    for (int i = 0; i < 16; i++) {
        add_bit_nec(encoder, symbols, &index, (address >> i) & 1);
    }
    
    // Command (8 bits LSB first)
    for (int i = 0; i < 8; i++) {
        add_bit_nec(encoder, symbols, &index, (command >> i) & 1);
    }
    
    // Command inverse (8 bits LSB first)
    uint8_t command_inv = ~command;
    for (int i = 0; i < 8; i++) {
        add_bit_nec(encoder, symbols, &index, (command_inv >> i) & 1);
    }
    
    // Stop bit
    symbols[index++] = make_symbol(encoder, NEC_STOP_BIT_US, 0);
    
    *ret_num_symbols = index;
    ESP_LOGD(TAG, "NEC Extended encoded: addr=0x%04X, cmd=0x%02X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_nec_repeat(ir_encoder_handle_t encoder,
                              rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 2, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for NEC repeat");
    
    // NEC repeat signal
    symbols[0] = make_symbol(encoder, NEC_REPEAT_HIGH_US, NEC_REPEAT_LOW_US);
    symbols[1] = make_symbol(encoder, NEC_STOP_BIT_US, 0);
    
    *ret_num_symbols = 2;
    ESP_LOGD(TAG, "NEC repeat encoded");
    
    return ESP_OK;
}

esp_err_t ir_encode_sony(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                        rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 13, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for Sony");
    
    size_t index = 0;
    
    // Header (start bit)
    symbols[index++] = make_symbol(encoder, SONY_HEADER_HIGH_US, SONY_HEADER_LOW_US);
    
    // Command (7 bits LSB first)
    for (int i = 0; i < 7; i++) {
        add_bit_sony(encoder, symbols, &index, (command >> i) & 1);
    }
    
    // Address (5 bits LSB first) - SIRC-12
    for (int i = 0; i < 5; i++) {
        add_bit_sony(encoder, symbols, &index, (address >> i) & 1);
    }
    
    *ret_num_symbols = index;
    ESP_LOGD(TAG, "Sony encoded: addr=0x%02X, cmd=0x%02X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_rc5(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                       rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 14, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for RC5");
    
    size_t index = 0;
    
    // Start bits (sempre 1,1)
    symbols[index++] = make_symbol(encoder, RC5_HALF_BIT_US, RC5_HALF_BIT_US);
    symbols[index++] = make_symbol(encoder, RC5_HALF_BIT_US, RC5_HALF_BIT_US);
    
    // Toggle bit (simplificado como 0)
    symbols[index++] = make_symbol(encoder, 0, RC5_BIT_DURATION_US);
    
    // Address (5 bits MSB first)
    for (int i = 4; i >= 0; i--) {
        if ((address >> i) & 1) {
            symbols[index++] = make_symbol(encoder, RC5_HALF_BIT_US, RC5_HALF_BIT_US);
        } else {
            symbols[index++] = make_symbol(encoder, 0, RC5_BIT_DURATION_US);
        }
    }
    
    // Command (6 bits MSB first)
    for (int i = 5; i >= 0; i--) {
        if ((command >> i) & 1) {
            symbols[index++] = make_symbol(encoder, RC5_HALF_BIT_US, RC5_HALF_BIT_US);
        } else {
            symbols[index++] = make_symbol(encoder, 0, RC5_BIT_DURATION_US);
        }
    }
    
    *ret_num_symbols = index;
    ESP_LOGD(TAG, "RC5 encoded: addr=0x%02X, cmd=0x%02X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_rc6(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                       rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 20, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for RC6");
    
    size_t index = 0;
    
    // Header
    symbols[index++] = make_symbol(encoder, RC6_HEADER_HIGH_US, RC6_HEADER_LOW_US);
    
    // Start bit
    symbols[index++] = make_symbol(encoder, RC6_HALF_BIT_US, RC6_HALF_BIT_US);
    
    // Mode bits (3 bits - mode 0)
    symbols[index++] = make_symbol(encoder, 0, RC6_BIT_DURATION_US); // Mode bit 0
    symbols[index++] = make_symbol(encoder, 0, RC6_BIT_DURATION_US); // Mode bit 1
    symbols[index++] = make_symbol(encoder, 0, RC6_BIT_DURATION_US); // Mode bit 2
    
    // Trailer bit
    symbols[index++] = make_symbol(encoder, RC6_BIT_DURATION_US, RC6_BIT_DURATION_US);
    
    // Address (8 bits)
    for (int i = 7; i >= 0; i--) {
        if ((address >> i) & 1) {
            symbols[index++] = make_symbol(encoder, RC6_HALF_BIT_US, RC6_HALF_BIT_US);
        } else {
            symbols[index++] = make_symbol(encoder, 0, RC6_BIT_DURATION_US);
        }
    }
    
    // Command (8 bits)
    for (int i = 7; i >= 0; i--) {
        if ((command >> i) & 1) {
            symbols[index++] = make_symbol(encoder, RC6_HALF_BIT_US, RC6_HALF_BIT_US);
        } else {
            symbols[index++] = make_symbol(encoder, 0, RC6_BIT_DURATION_US);
        }
    }
    
    *ret_num_symbols = index;
    ESP_LOGD(TAG, "RC6 encoded: addr=0x%02X, cmd=0x%02X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_samsung(ir_encoder_handle_t encoder, uint16_t address, uint8_t command,
                           rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 34, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for Samsung");
    
    size_t index = 0;
    
    // Header
    symbols[index++] = make_symbol(encoder, SAMSUNG_HEADER_HIGH_US, SAMSUNG_HEADER_LOW_US);
    
    // Address (16 bits LSB first)
    for (int i = 0; i < 16; i++) {
        add_bit_samsung(encoder, symbols, &index, (address >> i) & 1);
    }
    
    // Command (8 bits LSB first)
    for (int i = 0; i < 8; i++) {
        add_bit_samsung(encoder, symbols, &index, (command >> i) & 1);
    }
    
    // Command inverse (8 bits LSB first)
    uint8_t command_inv = ~command;
    for (int i = 0; i < 8; i++) {
        add_bit_samsung(encoder, symbols, &index, (command_inv >> i) & 1);
    }
    
    // Stop bit
    symbols[index++] = make_symbol(encoder, SAMSUNG_STOP_BIT_US, 0);
    
    *ret_num_symbols = index;
    ESP_LOGD(TAG, "Samsung encoded: addr=0x%04X, cmd=0x%02X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_panasonic(ir_encoder_handle_t encoder, uint16_t address, uint32_t command,
                             rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 49, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for Panasonic");
    
    size_t index = 0;
    
    // Header
    symbols[index++] = make_symbol(encoder, PANASONIC_HEADER_HIGH_US, PANASONIC_HEADER_LOW_US);
    
    // Address (16 bits LSB first)
    for (int i = 0; i < 16; i++) {
        add_bit_panasonic(encoder, symbols, &index, (address >> i) & 1);
    }
    
    // Command (32 bits LSB first)
    for (int i = 0; i < 32; i++) {
        add_bit_panasonic(encoder, symbols, &index, (command >> i) & 1);
    }
    
    *ret_num_symbols = index;
    //ESP_LOGD(TAG, "Panasonic encoded: addr=0x%04X, cmd=0x%08X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_jvc(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                       rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 17, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for JVC");
    
    size_t index = 0;
    
    // Header
    symbols[index++] = make_symbol(encoder, JVC_HEADER_HIGH_US, JVC_HEADER_LOW_US);
    
    // Address (8 bits LSB first)
    for (int i = 0; i < 8; i++) {
        add_bit_jvc(encoder, symbols, &index, (address >> i) & 1);
    }
    
    // Command (8 bits LSB first)
    for (int i = 0; i < 8; i++) {
        add_bit_jvc(encoder, symbols, &index, (command >> i) & 1);
    }
    
    *ret_num_symbols = index;
    ESP_LOGD(TAG, "JVC encoded: addr=0x%02X, cmd=0x%02X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_lg(ir_encoder_handle_t encoder, uint8_t address, uint16_t command,
                      rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 25, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for LG");
    
    size_t index = 0;
    
    // Header
    symbols[index++] = make_symbol(encoder, LG_HEADER_HIGH_US, LG_HEADER_LOW_US);
    
    // Address (8 bits LSB first)
    for (int i = 0; i < 8; i++) {
        add_bit_lg(encoder, symbols, &index, (address >> i) & 1);
    }
    
    // Command (16 bits LSB first)
    for (int i = 0; i < 16; i++) {
        add_bit_lg(encoder, symbols, &index, (command >> i) & 1);
    }
    
    *ret_num_symbols = index;
    ESP_LOGD(TAG, "LG encoded: addr=0x%02X, cmd=0x%04X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_mitsubishi(ir_encoder_handle_t encoder, uint16_t address, uint16_t command,
                              rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 33, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for Mitsubishi");
    
    size_t index = 0;
    
    // Header
    symbols[index++] = make_symbol(encoder, MITSUBISHI_HEADER_HIGH_US, MITSUBISHI_HEADER_LOW_US);
    
    // Address (16 bits LSB first)
    for (int i = 0; i < 16; i++) {
        add_bit_mitsubishi(encoder, symbols, &index, (address >> i) & 1);
    }
    
    // Command (16 bits LSB first)
    for (int i = 0; i < 16; i++) {
        add_bit_mitsubishi(encoder, symbols, &index, (command >> i) & 1);
    }
    
    *ret_num_symbols = index;
    ESP_LOGD(TAG, "Mitsubishi encoded: addr=0x%04X, cmd=0x%04X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_sharp(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                         rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 15, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for Sharp");
    
    size_t index = 0;
    
    // Sharp protocol doesn't have header, starts directly with data
    // Address (5 bits MSB first)
    for (int i = 4; i >= 0; i--) {
        add_bit_sharp(encoder, symbols, &index, (address >> i) & 1);
    }
    
    // Command (8 bits MSB first)
    for (int i = 7; i >= 0; i--) {
        add_bit_sharp(encoder, symbols, &index, (command >> i) & 1);
    }
    
    // Expansion bit (always 0)
    add_bit_sharp(encoder, symbols, &index, 0);
    
    // Check bit (even parity)
    uint8_t check = 0;
    for (int i = 0; i < 5; i++) {
        if ((address >> i) & 1) check++;
    }
    for (int i = 0; i < 8; i++) {
        if ((command >> i) & 1) check++;
    }
    add_bit_sharp(encoder, symbols, &index, check & 1);
    
    *ret_num_symbols = index;
    ESP_LOGD(TAG, "Sharp encoded: addr=0x%02X, cmd=0x%02X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_dish(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                        rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 17, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for Dish");
    
    size_t index = 0;
    
    // Header
    symbols[index++] = make_symbol(encoder, DISH_HEADER_HIGH_US, DISH_HEADER_LOW_US);
    
    // Address (8 bits MSB first)
    for (int i = 7; i >= 0; i--) {
        add_bit_dish(encoder, symbols, &index, (address >> i) & 1);
    }
    
    // Command (8 bits MSB first)
    for (int i = 7; i >= 0; i--) {
        add_bit_dish(encoder, symbols, &index, (command >> i) & 1);
    }
    
    *ret_num_symbols = index;
    ESP_LOGD(TAG, "Dish encoded: addr=0x%02X, cmd=0x%02X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_aiwa(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                        rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 17, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for Aiwa");
    
    size_t index = 0;
    
    // Header
    symbols[index++] = make_symbol(encoder, AIWA_HEADER_HIGH_US, AIWA_HEADER_LOW_US);
    
    // Address (8 bits LSB first)
    for (int i = 0; i < 8; i++) {
        add_bit_aiwa(encoder, symbols, &index, (address >> i) & 1);
    }
    
    // Command (8 bits LSB first)
    for (int i = 0; i < 8; i++) {
        add_bit_aiwa(encoder, symbols, &index, (command >> i) & 1);
    }
    
    *ret_num_symbols = index;
    ESP_LOGD(TAG, "Aiwa encoded: addr=0x%02X, cmd=0x%02X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_denon(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                         rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 17, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for Denon");
    
    size_t index = 0;
    
    // Header
    symbols[index++] = make_symbol(encoder, DENON_HEADER_HIGH_US, DENON_HEADER_LOW_US);
    
    // Address (8 bits LSB first)
    for (int i = 0; i < 8; i++) {
        add_bit_denon(encoder, symbols, &index, (address >> i) & 1);
    }
    
    // Command (8 bits LSB first)
    for (int i = 0; i < 8; i++) {
        add_bit_denon(encoder, symbols, &index, (command >> i) & 1);
    }
    
    *ret_num_symbols = index;
    ESP_LOGD(TAG, "Denon encoded: addr=0x%02X, cmd=0x%02X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_kaseikyo(ir_encoder_handle_t encoder, uint16_t address, uint32_t command,
                            rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 49, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for Kaseikyo");
    
    size_t index = 0;
    
    // Header
    symbols[index++] = make_symbol(encoder, KASEIKYO_HEADER_HIGH_US, KASEIKYO_HEADER_LOW_US);
    
    // Address (16 bits LSB first)
    for (int i = 0; i < 16; i++) {
        add_bit_kaseikyo(encoder, symbols, &index, (address >> i) & 1);
    }
    
    // Command (32 bits LSB first)
    for (int i = 0; i < 32; i++) {
        add_bit_kaseikyo(encoder, symbols, &index, (command >> i) & 1);
    }
    
    *ret_num_symbols = index;
    //ESP_LOGD(TAG, "Kaseikyo encoded: addr=0x%04X, cmd=0x%08X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_whynter(ir_encoder_handle_t encoder, uint8_t address, uint8_t command,
                           rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 33, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for Whynter");
    
    size_t index = 0;
    
    // Header
    symbols[index++] = make_symbol(encoder, WHYNTER_HEADER_HIGH_US, WHYNTER_HEADER_LOW_US);
    
    // Address (8 bits LSB first)
    for (int i = 0; i < 8; i++) {
        add_bit_whynter(encoder, symbols, &index, (address >> i) & 1);
    }
    
    // Command (8 bits LSB first)
    for (int i = 0; i < 8; i++) {
        add_bit_whynter(encoder, symbols, &index, (command >> i) & 1);
    }
    
    // Repeat the same data for Whynter protocol
    for (int i = 0; i < 8; i++) {
        add_bit_whynter(encoder, symbols, &index, (address >> i) & 1);
    }
    
    for (int i = 0; i < 8; i++) {
        add_bit_whynter(encoder, symbols, &index, (command >> i) & 1);
    }
    
    *ret_num_symbols = index;
    ESP_LOGD(TAG, "Whynter encoded: addr=0x%02X, cmd=0x%02X, symbols=%zu", address, command, index);
    
    return ESP_OK;
}

esp_err_t ir_encode_coolix(ir_encoder_handle_t encoder, uint32_t command,
                          rmt_symbol_word_t *symbols, size_t symbols_size, size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && symbols && ret_num_symbols, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(symbols_size >= 49, ESP_ERR_INVALID_SIZE, TAG, "buffer too small for Coolix");
    
    size_t index = 0;
    
    // Header
    symbols[index++] = make_symbol(encoder, COOLIX_HEADER_HIGH_US, COOLIX_HEADER_LOW_US);
    
    // Command (24 bits LSB first)
    for (int i = 0; i < 24; i++) {
        add_bit_coolix(encoder, symbols, &index, (command >> i) & 1);
    }
    
    // Footer
    symbols[index++] = make_symbol(encoder, COOLIX_BIT_ZERO_HIGH_US, COOLIX_HEADER_LOW_US);
    
    // Repeat the same data
    symbols[index++] = make_symbol(encoder, COOLIX_HEADER_HIGH_US, COOLIX_HEADER_LOW_US);
    
    for (int i = 0; i < 24; i++) {
        add_bit_coolix(encoder, symbols, &index, (command >> i) & 1);
    }
    
    *ret_num_symbols = index;
    //ESP_LOGD(TAG, "Coolix encoded: cmd=0x%08X, symbols=%zu", command, index);
    
    return ESP_OK;
}

esp_err_t ir_encoder_encode(ir_encoder_handle_t encoder, 
                           const ir_command_t *command,
                           rmt_symbol_word_t *symbols,
                           size_t symbols_size,
                           size_t *ret_num_symbols)
{
    ESP_RETURN_ON_FALSE(encoder && command && symbols && ret_num_symbols, 
                       ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    
    switch (command->protocol) {
        case IR_PROTOCOL_NEC:
            if (command->repeat) {
                return ir_encode_nec_repeat(encoder, symbols, symbols_size, ret_num_symbols);
            } else {
                return ir_encode_nec(encoder, command->address & 0xFF, command->command & 0xFF, 
                                   symbols, symbols_size, ret_num_symbols);
            }
            
        case IR_PROTOCOL_NEC_EXT:
            return ir_encode_nec_ext(encoder, command->address & 0xFFFF, command->command & 0xFF,
                                   symbols, symbols_size, ret_num_symbols);
            
        case IR_PROTOCOL_SONY:
            return ir_encode_sony(encoder, command->address & 0x1F, command->command & 0x7F,
                                symbols, symbols_size, ret_num_symbols);
            
        case IR_PROTOCOL_RC5:
            return ir_encode_rc5(encoder, command->address & 0x1F, command->command & 0x3F,
                               symbols, symbols_size, ret_num_symbols);
            
        case IR_PROTOCOL_RC6:
            return ir_encode_rc6(encoder, command->address & 0xFF, command->command & 0xFF,
                               symbols, symbols_size, ret_num_symbols);
            
        case IR_PROTOCOL_SAMSUNG:
            return ir_encode_samsung(encoder, command->address & 0xFFFF, command->command & 0xFF,
                                   symbols, symbols_size, ret_num_symbols);
            
        case IR_PROTOCOL_PANASONIC:
            return ir_encode_panasonic(encoder, command->address & 0xFFFF, command->command,
                                     symbols, symbols_size, ret_num_symbols);
            
        case IR_PROTOCOL_JVC:
            return ir_encode_jvc(encoder, command->address & 0xFF, command->command & 0xFF,
                               symbols, symbols_size, ret_num_symbols);
            
        case IR_PROTOCOL_LG:
            return ir_encode_lg(encoder, command->address & 0xFF, command->command & 0xFFFF,
                              symbols, symbols_size, ret_num_symbols);
            
        case IR_PROTOCOL_MITSUBISHI:
            return ir_encode_mitsubishi(encoder, command->address & 0xFFFF, command->command & 0xFFFF,
                                      symbols, symbols_size, ret_num_symbols);
            
        case IR_PROTOCOL_SHARP:
            return ir_encode_sharp(encoder, command->address & 0x1F, command->command & 0xFF,
                                 symbols, symbols_size, ret_num_symbols);
            
        case IR_PROTOCOL_DISH:
            return ir_encode_dish(encoder, command->address & 0xFF, command->command & 0xFF,
                                symbols, symbols_size, ret_num_symbols);
            
        case IR_PROTOCOL_AIWA:
            return ir_encode_aiwa(encoder, command->address & 0xFF, command->command & 0xFF,
                                symbols, symbols_size, ret_num_symbols);
            
        case IR_PROTOCOL_DENON:
            return ir_encode_denon(encoder, command->address & 0xFF, command->command & 0xFF,
                                 symbols, symbols_size, ret_num_symbols);
            
        case IR_PROTOCOL_KASEIKYO:
            return ir_encode_kaseikyo(encoder, command->address & 0xFFFF, command->command,
                                    symbols, symbols_size, ret_num_symbols);
            
        case IR_PROTOCOL_WHYNTER:
            return ir_encode_whynter(encoder, command->address & 0xFF, command->command & 0xFF,
                                   symbols, symbols_size, ret_num_symbols);
            
        case IR_PROTOCOL_COOLIX:
            return ir_encode_coolix(encoder, command->command & 0xFFFFFF,
                                  symbols, symbols_size, ret_num_symbols);
            
        default:
            ESP_LOGE(TAG, "Unsupported protocol: %d", command->protocol);
            return ESP_ERR_NOT_SUPPORTED;
    }
}

// Funções auxiliares

ir_protocol_t ir_protocol_from_string(const char *protocol_str)
{
    if (!protocol_str) return IR_PROTOCOL_GENERIC;
    
    if (strcasecmp(protocol_str, "NEC") == 0) return IR_PROTOCOL_NEC;
    if (strcasecmp(protocol_str, "NEC_EXT") == 0) return IR_PROTOCOL_NEC_EXT;
    if (strcasecmp(protocol_str, "SONY") == 0) return IR_PROTOCOL_SONY;
    if (strcasecmp(protocol_str, "RC5") == 0) return IR_PROTOCOL_RC5;
    if (strcasecmp(protocol_str, "RC6") == 0) return IR_PROTOCOL_RC6;
    if (strcasecmp(protocol_str, "SAMSUNG") == 0) return IR_PROTOCOL_SAMSUNG;
    if (strcasecmp(protocol_str, "PANASONIC") == 0) return IR_PROTOCOL_PANASONIC;
    if (strcasecmp(protocol_str, "JVC") == 0) return IR_PROTOCOL_JVC;
    if (strcasecmp(protocol_str, "LG") == 0) return IR_PROTOCOL_LG;
    if (strcasecmp(protocol_str, "MITSUBISHI") == 0) return IR_PROTOCOL_MITSUBISHI;
    if (strcasecmp(protocol_str, "SHARP") == 0) return IR_PROTOCOL_SHARP;
    if (strcasecmp(protocol_str, "DISH") == 0) return IR_PROTOCOL_DISH;
    if (strcasecmp(protocol_str, "AIWA") == 0) return IR_PROTOCOL_AIWA;
    if (strcasecmp(protocol_str, "DENON") == 0) return IR_PROTOCOL_DENON;
    if (strcasecmp(protocol_str, "KASEIKYO") == 0) return IR_PROTOCOL_KASEIKYO;
    if (strcasecmp(protocol_str, "WHYNTER") == 0) return IR_PROTOCOL_WHYNTER;
    if (strcasecmp(protocol_str, "COOLIX") == 0) return IR_PROTOCOL_COOLIX;
    
    return IR_PROTOCOL_GENERIC;
}

const char* ir_protocol_to_string(ir_protocol_t protocol)
{
    if (protocol >= IR_PROTOCOL_MAX) return "UNKNOWN";
    return protocol_info_table[protocol].name;
}

const ir_protocol_info_t* ir_get_protocol_info(ir_protocol_t protocol)
{
    if (protocol >= IR_PROTOCOL_MAX) return NULL;
    return &protocol_info_table[protocol];
}