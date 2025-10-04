#ifndef PROTOCOL_NEC_H
#define PROTOCOL_NEC_H

#include "driver/rmt_encoder.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estrutura de scan code NEC
 */
typedef struct {
    uint16_t address;  ///< Endereço NEC (16 bits)
    uint16_t command;  ///< Comando NEC (16 bits)
} ir_nec_scan_code_t;

/**
 * @brief Timings do protocolo NEC
 */
#define NEC_LEADING_CODE_DURATION_0  9000
#define NEC_LEADING_CODE_DURATION_1  4500
#define NEC_PAYLOAD_ZERO_DURATION_0  560
#define NEC_PAYLOAD_ZERO_DURATION_1  560
#define NEC_PAYLOAD_ONE_DURATION_0   560
#define NEC_PAYLOAD_ONE_DURATION_1   1690
#define NEC_REPEAT_CODE_DURATION_0   9000
#define NEC_REPEAT_CODE_DURATION_1   2250

#define EXAMPLE_IR_NEC_DECODE_MARGIN 200  ///< Margem de erro para decodificação (us)

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_NEC_H