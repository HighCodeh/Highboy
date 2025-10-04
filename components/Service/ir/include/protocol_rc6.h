#ifndef PROTOCOL_RC6_H
#define PROTOCOL_RC6_H

#include "driver/rmt_encoder.h"
#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estrutura de scan code RC6
 */
typedef struct {
    uint8_t address;  ///< Endereço RC6 (8 bits)
    uint8_t command;  ///< Comando RC6 (8 bits)
    uint8_t toggle;   ///< Bit de toggle RC6 (0 ou 1)
} ir_rc6_scan_code_t;

/**
 * @brief Timings do protocolo RC6 (em microsegundos)
 */
#define RC6_LEADING_CODE_DURATION_0   2666
#define RC6_LEADING_CODE_DURATION_1   889
#define RC6_PAYLOAD_ZERO_DURATION_0   444
#define RC6_PAYLOAD_ZERO_DURATION_1   444
#define RC6_PAYLOAD_ONE_DURATION_0    444
#define RC6_PAYLOAD_ONE_DURATION_1    444
#define RC6_TOGGLE_DURATION_0         889
#define RC6_TOGGLE_DURATION_1         889

#define EXAMPLE_IR_RC6_DECODE_MARGIN  200  ///< Margem de erro para decodificação (us)

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_RC6_H