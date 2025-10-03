#include "ir_common.h"
#include "ir_storage.h"
#include <string.h>

static const char *TAG = "ir_tx";

esp_err_t ir_tx_init(ir_context_t *ctx)
{
    esp_err_t ret = ESP_OK;
    
    ESP_LOGI(TAG, "create RMT TX channel");
    rmt_tx_channel_config_t tx_channel_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = EXAMPLE_IR_RESOLUTION_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
        .gpio_num = EXAMPLE_IR_TX_GPIO_NUM,
    };
    
    ret = rmt_new_tx_channel(&tx_channel_cfg, &ctx->tx_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create TX channel: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "modulate carrier to TX channel");
    rmt_carrier_config_t carrier_cfg = {
        .duty_cycle = 0.33,
        .frequency_hz = 38000,
    };
    ret = rmt_apply_carrier(ctx->tx_channel, &carrier_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to apply carrier: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "install IR encoder (NEC)");
    ir_encoder_config_t enc_cfg = {
        .protocol   = IR_PROTOCOL_NEC,
        .config.nec = { .resolution = EXAMPLE_IR_RESOLUTION_HZ },
    };
    ret = rmt_new_ir_encoder(&enc_cfg, &ctx->encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create encoder: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "enable RMT TX channel");
    ret = rmt_enable(ctx->tx_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable TX channel: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "TX module initialized successfully");
    return ret;
}

esp_err_t ir_tx_deinit(ir_context_t *ctx)
{
    if (!ctx) {
        ESP_LOGE(TAG, "Invalid context");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;

    // Deletar encoder se existe
    if (ctx->encoder) {
        ESP_LOGI(TAG, "Deleting IR encoder");
        esp_err_t del_enc_ret = rmt_del_encoder(ctx->encoder);
        if (del_enc_ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete encoder: %s", esp_err_to_name(del_enc_ret));
            ret = del_enc_ret;
        }
        ctx->encoder = NULL;
    }

    // Desabilitar canal TX se existe
    if (ctx->tx_channel) {
        ESP_LOGI(TAG, "Disabling RMT TX channel");
        esp_err_t disable_ret = rmt_disable(ctx->tx_channel);
        if (disable_ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to disable TX channel: %s", esp_err_to_name(disable_ret));
            ret = disable_ret;
        }

        ESP_LOGI(TAG, "Deleting RMT TX channel");
        esp_err_t del_ret = rmt_del_channel(ctx->tx_channel);
        if (del_ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete TX channel: %s", esp_err_to_name(del_ret));
            ret = del_ret;
        }
        ctx->tx_channel = NULL;
    }

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "TX module deinitialized successfully");
    }

    return ret;
}

esp_err_t ir_tx_send_nec(ir_context_t *ctx, uint16_t address, uint16_t command)
{
    if (!ctx || !ctx->tx_channel || !ctx->encoder) {
        ESP_LOGE(TAG, "TX not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    const ir_nec_scan_code_t scan_code = {
        .address = address,
        .command = command,
    };
    
    rmt_transmit_config_t transmit_config = { 
        .loop_count = 0 
    };
    
    esp_err_t ret = rmt_transmit(ctx->tx_channel, ctx->encoder, &scan_code, sizeof(scan_code), &transmit_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to transmit: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Sent NEC code - Address: 0x%04X, Command: 0x%04X", address, command);
    }
    
    return ret;
}

bool ir_tx_send_from_file(const char* filename) {

    ir_context_t ir_ctx = {0};
    esp_err_t ret = ir_tx_init(&ir_ctx);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TX: %s", esp_err_to_name(ret));
        return false;
    }

    // Carrega o código do arquivo
    ir_code_t ir_code;
    if (!ir_load(filename, &ir_code)) {
        ESP_LOGE(TAG, "Falha ao carregar código do arquivo: %s", filename);
        return false;
    }

    ESP_LOGI(TAG, "Transmitindo código do arquivo %s: Protocol=%s, Address=0x%08lX, Command=0x%08lX", 
             filename, ir_code.protocol, ir_code.address, ir_code.command);

    ret = ESP_FAIL;

    // Identifica o protocolo e chama a função apropriada
    if (strcmp(ir_code.protocol, "NEC") == 0) {
        ret = ir_tx_send_nec(&ir_ctx, ir_code.address, ir_code.command);
    }
    else if (strcmp(ir_code.protocol, "Samsung32") == 0) {
        //ret = ir_tx_send_samsung32(ir_ctx, ir_code.address, ir_code.command);
    }
    else if (strcmp(ir_code.protocol, "RC5") == 0) {
        //ret = ir_tx_send_rc5(ir_ctx, ir_code.address, ir_code.command);
    }
    else if (strcmp(ir_code.protocol, "RC6") == 0) {
        //ret = ir_tx_send_rc6(ir_ctx, ir_code.address, ir_code.command);
    }
    else if (strcmp(ir_code.protocol, "SIRC") == 0) {
        //ret = ir_tx_send_sirc(ir_ctx, ir_code.address, ir_code.command);
    }
    else {
        ESP_LOGE(TAG, "Protocolo não suportado: %s", ir_code.protocol);
        return false;
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao transmitir código IR: %s", esp_err_to_name(ret));
        return false;
    }

    ESP_LOGI(TAG, "Código IR transmitido com sucesso!");

    vTaskDelay(pdMS_TO_TICKS(500));

    ir_tx_deinit(&ir_ctx);

    return true;
}