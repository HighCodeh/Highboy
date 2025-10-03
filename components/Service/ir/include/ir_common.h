#ifndef IR_COMMON_H
#define IR_COMMON_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"
#include "ir_encoder.h"

// Definições comuns
#define EXAMPLE_IR_RESOLUTION_HZ     1000000 // 1MHz resolution, 1 tick = 1us
#define EXAMPLE_IR_TX_GPIO_NUM       21
#define EXAMPLE_IR_RX_GPIO_NUM       2

// Estrutura para dados compartilhados
typedef struct {
    rmt_channel_handle_t tx_channel;
    rmt_channel_handle_t rx_channel;
    rmt_encoder_handle_t encoder;
    QueueHandle_t receive_queue;
} ir_context_t;

// Funções públicas
esp_err_t ir_tx_init(ir_context_t *ctx);
esp_err_t ir_rx_init(ir_context_t *ctx);
esp_err_t ir_tx_send_nec(ir_context_t *ctx, uint16_t address, uint16_t command);
void ir_rx_start_receive(ir_context_t *ctx);
bool ir_rx_wait_for_data(ir_context_t *ctx, rmt_rx_done_event_data_t *rx_data, uint32_t timeout_ms);
bool ir_tx_send_from_file(const char* filename);

// Parser NEC (assumindo que existe)
void parse_nec_frame(rmt_symbol_word_t *symbols, size_t num_symbols, const char* filename);

bool ir_receive(const char* filename, uint32_t timeout_ms);

#endif // IR_COMMON_H