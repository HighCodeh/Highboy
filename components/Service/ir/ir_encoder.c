#include "ir_encoder.h"
#include "esp_err.h"

esp_err_t rmt_new_ir_encoder(const ir_encoder_config_t *cfg, rmt_encoder_handle_t *ret_encoder)
{
    if (!cfg || !ret_encoder) {
        return ESP_ERR_INVALID_ARG;
    }
    switch (cfg->protocol) {
    case IR_PROTOCOL_NEC:
        return rmt_new_ir_nec_encoder(&cfg->config.nec, ret_encoder);
    default:
        return ESP_ERR_NOT_SUPPORTED;
    }
}
