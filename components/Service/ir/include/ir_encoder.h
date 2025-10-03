#pragma once

#include "driver/rmt_encoder.h"
#include "protocol_nec.h"

typedef enum {
    IR_PROTOCOL_NEC = 0,
    // IR_PROTOCOL_RC5,
    // IR_PROTOCOL_SONY,
    // etc.
} ir_protocol_t;

typedef struct {
    ir_protocol_t        protocol;
    union {
        ir_nec_encoder_config_t nec;
        // add other protocol configs here
    } config;
} ir_encoder_config_t;

/**
 * @brief Create a new RMT encoder for the selected IR protocol.
 *
 * @param[in]  cfg          Generic IR encoder configuration (select protocol + params).
 * @param[out] ret_encoder  Receives the newly created rmt_encoder_handle_t.
 * @return
 *     - ESP_ERR_INVALID_ARG if arguments are NULL
 *     - ESP_ERR_NOT_SUPPORTED if protocol isn’t implemented
 *     - ESP_OK on success
 */
esp_err_t rmt_new_ir_encoder(const ir_encoder_config_t *cfg, rmt_encoder_handle_t *ret_encoder);
