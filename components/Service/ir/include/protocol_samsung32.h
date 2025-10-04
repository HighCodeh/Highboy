#ifndef PROTOCOL_SAMSUNG32_H
#define PROTOCOL_SAMSUNG32_H

#include "driver/rmt_encoder.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estrutura de scan code Samsung (32 bits)
 */
typedef struct {
    uint32_t data;  ///< Dados Samsung completos (32 bits)
} ir_samsung32_scan_code_t;

/**
 * @brief Timings do protocolo Samsung (em microsegundos)
 */
#define SAMSUNG32_LEADING_CODE_DURATION_0  4500
#define SAMSUNG32_LEADING_CODE_DURATION_1  4500
#define SAMSUNG32_PAYLOAD_ZERO_DURATION_0  560
#define SAMSUNG32_PAYLOAD_ZERO_DURATION_1  560
#define SAMSUNG32_PAYLOAD_ONE_DURATION_0   560
#define SAMSUNG32_PAYLOAD_ONE_DURATION_1   1690

#define EXAMPLE_IR_SAMSUNG32_DECODE_MARGIN 200  ///< Margem de erro para decodificação (us)

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_SAMSUNG32_H