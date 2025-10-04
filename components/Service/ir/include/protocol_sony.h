#ifndef PROTOCOL_SONY_H
#define PROTOCOL_SONY_H

#include "driver/rmt_encoder.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estrutura de scan code Sony SIRC
 */
typedef struct {
    uint16_t address;  ///< Endereço Sony (5, 8 ou 13 bits dependendo do modo)
    uint8_t command;   ///< Comando Sony (7 bits)
    uint8_t bits;      ///< Número de bits no frame (12, 15 ou 20)
} ir_sony_scan_code_t;

/**
 * @brief Timings do protocolo Sony SIRC (em microsegundos)
 */
#define SONY_LEADING_CODE_DURATION    2400
#define SONY_PAYLOAD_ZERO_DURATION    600
#define SONY_PAYLOAD_ONE_DURATION     1200
#define SONY_BIT_PERIOD               600

#define EXAMPLE_IR_SONY_DECODE_MARGIN 200  ///< Margem de erro para decodificação (us)

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_SONY_H