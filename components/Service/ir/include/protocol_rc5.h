#ifndef PROTOCOL_RC5_H
#define PROTOCOL_RC5_H

#include "driver/rmt_encoder.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estrutura de scan code RC5
 */
typedef struct {
    uint8_t address;  ///< Endereço RC5 (5 bits)
    uint8_t command;  ///< Comando RC5 (6 bits)
    uint8_t toggle;   ///< Bit de toggle RC5 (0 ou 1)
} ir_rc5_scan_code_t;

/**
 * @brief Timings do protocolo RC5 (em microsegundos)
 */
#define RC5_BIT_DURATION              889
#define RC5_LEADING_CODE_DURATION_0   889
#define RC5_LEADING_CODE_DURATION_1   889

#define EXAMPLE_IR_RC5_DECODE_MARGIN  200  ///< Margem de erro para decodificação (us)

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_RC5_H