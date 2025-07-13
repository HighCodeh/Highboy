#include "ir_receiver.h"
#include "driver/rmt_tx.h"  // Novo driver para transmissão
#include "driver/rmt_rx.h"  // Novo driver para recepção
#include "soc/rtc.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"  // Biblioteca para fila no FreeRTOS

rmt_channel_handle_t rx_chan = NULL;
QueueHandle_t receive_queue = NULL;
static bool rx_chan_enabled = false;
static bool queue_created = false;

//Callback do RX
bool on_recv_done(rmt_channel_handle_t chan, const rmt_rx_done_event_data_t *edata, void *user_data) {
    // Enviar os dados recebidos para a fila (operação segura em ISR)
    BaseType_t high_task_awoken = pdFALSE;
    
    if (receive_queue != NULL) {
        // Usar versão FromISR para operações em contexto de interrupção
        xQueueSendFromISR(receive_queue, edata, &high_task_awoken);
    }
    
    // Retornar se uma tarefa de maior prioridade foi desbloqueada
    return high_task_awoken == pdTRUE;
}

void rmt_rx_enable(){

    if (!rx_chan_enabled) {
        ESP_ERROR_CHECK(rmt_enable(rx_chan));
        rx_chan_enabled = true;
        ESP_LOGI("RMT", "Canal RX habilitado.");
    }

}

void rmt_rx_disable(){

    if (rx_chan_enabled) {
        ESP_ERROR_CHECK(rmt_disable(rx_chan));
        rx_chan_enabled = false;
        ESP_LOGI("RMT", "Canal RX desabilitado.");
    }
}

bool rmt_rx_delete() {
    if (rx_chan_enabled) {
        ESP_ERROR_CHECK(rmt_disable(rx_chan));  // desabilita antes de deletar
        rx_chan_enabled = false;
    }
    
    esp_err_t err_rx_del = rmt_del_channel(rx_chan);
    if (err_rx_del != ESP_OK) {
        ESP_LOGE("RMT", "Falha ao desalocar canal RX: %s", esp_err_to_name(err_rx_del));
        return false;
    }

    ESP_LOGI("RMT", "Canal RX desalocado com sucesso.");
    return true;
}

bool rmt_rx_init(){

    rmt_rx_channel_config_t rx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,            // Fonte de clock padrão
        .gpio_num = RX_GPIO_NUM,                             // Usando o GPIO 2 para o receptor IR
        .mem_block_symbols = MEM_BLOCK_SYMBOLS,                   // Tamanho do bloco de memória (64 símbolos)
        .resolution_hz = RESOLUTION_HZ,          // Resolução de 1 MHz (1 tick = 1 µs)
        .flags.invert_in = FLAGS_INVERT_IN,                  // Não inverter o sinal de entrada
        .flags.with_dma = FLAGS_WITH_DMA,                   // Não usar DMA
    };

    // Alocando e inicializando o canal RX
    esp_err_t err_rx = rmt_new_rx_channel(&rx_chan_config, &rx_chan);
    if (err_rx != ESP_OK) {
        ESP_LOGE("RMT", "Falha ao criar canal RX: %s", esp_err_to_name(err_rx));
        return false;
    }
    ESP_LOGI("RMT", "Canal RX configurado com sucesso.");

    rmt_carrier_config_t rx_carrier_cfg = {
        .duty_cycle = 0.33,                 // Ciclo de trabalho de 33%
        .frequency_hz = 25000,              // Frequência de 25 KHz (ligeiramente abaixo da frequência de TX)
        .flags.polarity_active_low = false, // A portadora será modulada para o nível alto
    };

    // Demodulação da portadora no canal RX
    esp_err_t err_rx_carrier = rmt_apply_carrier(rx_chan, &rx_carrier_cfg);
    if (err_rx_carrier != ESP_OK) {
        ESP_LOGE("RMT", "Falha ao aplicar demodulação de portadora no RX: %s", esp_err_to_name(err_rx_carrier));
        return false;
    }
    ESP_LOGI("RMT", "Demodulação de portadora aplicada no canal RX.");

    // Registrando o callback no canal RX
    rmt_rx_event_callbacks_t rx_callbacks = {
        .on_recv_done = on_recv_done,  // Registra o callback para o evento de recepção concluída
    };

    esp_err_t err_rx_callbacks = rmt_rx_register_event_callbacks(rx_chan, &rx_callbacks, NULL);
    if (err_rx_callbacks != ESP_OK) {
        ESP_LOGE("RMT", "Falha ao registrar callbacks do canal RX: %s", esp_err_to_name(err_rx_callbacks));
        return false;
    }

    //rmt_rx_enable();
    return true;
}

bool rmt_rx_receive_start_queue(){
    if (!queue_created) {
        receive_queue = xQueueCreate(5, sizeof(rmt_rx_done_event_data_t));
        if (receive_queue == NULL) {
            ESP_LOGE("RMT", "Falha ao criar a fila de recepção.");
            return false;
        }
        queue_created = true;
    }
    return true;
}

void rmt_rx_receive_stop_queue(){
    if (queue_created) {
        vQueueDelete(receive_queue);
        receive_queue = NULL;
        queue_created = false;
    }
}

rmt_receive_config_t rmt_rx_receive_config(){

    rmt_receive_config_t receive_config = {
        .signal_range_min_ns = 500,         // Menor duração: 0.5µs
        .signal_range_max_ns = 20000000,    // Maior duração: 20ms
    };

    return receive_config;

}

esp_err_t rmt_rx_start_receive(rmt_symbol_word_t *raw_symbols_receive, size_t max_symbols) {

    rmt_rx_receive_start_queue();

    rmt_rx_enable();
    rmt_receive_config_t cfg = rmt_rx_receive_config();

    esp_err_t err_receive = rmt_receive(rx_chan, raw_symbols_receive, max_symbols * sizeof(rmt_symbol_word_t), &cfg);
    if (err_receive != ESP_OK) {
        ESP_LOGE("RMT", "Erro ao iniciar recepção: %s", esp_err_to_name(err_receive));
    }

    return err_receive;

}

bool rmt_rx_receive_repeat(rmt_symbol_word_t *raw_symbols_receive){

    while(true){

        ESP_LOGI("RMT", "INICIANDO RECEPÇÃO");

        rmt_disable(rx_chan);
        vTaskDelay(pdMS_TO_TICKS(10));

        esp_err_t err_receive = rmt_rx_start_receive(raw_symbols_receive, 64);

        if (err_receive != ESP_OK) {
            ESP_LOGE("RMT", "Erro ao iniciar recepção: %s", esp_err_to_name(err_receive));
            return false;
        }

        ESP_LOGI("RMT", "AGUARDANDO SINAL IR");

        rmt_rx_done_event_data_t rx_data;
        if (xQueueReceive(receive_queue, &rx_data, pdMS_TO_TICKS(10000)) == pdTRUE) {  // Alterado para 10 segundos

            ESP_LOGI("RMT", "✓ SUCESSO! Sinal recebido com %d símbolos", rx_data.num_symbols);
            
            // Mostrar alguns símbolos
            int symbols_to_show = (rx_data.num_symbols < 4) ? rx_data.num_symbols : 4;
            for (int i = 0; i < symbols_to_show; i++) {
                ESP_LOGI("RMT", "  Símbolo[%d]: H:%d(%dus) L:%d(%dus)", 
                    i,
                    rx_data.received_symbols[i].level0, rx_data.received_symbols[i].duration0,
                    rx_data.received_symbols[i].level1, rx_data.received_symbols[i].duration1);
            }
            
            vTaskDelay(pdMS_TO_TICKS(10));
            
        } else {
            xQueueReset(receive_queue);
        }

    }

    rmt_rx_receive_stop_queue();

}

bool rmt_rx_receive_once(rmt_symbol_word_t *raw_symbols_receive, size_t max_symbols, size_t *symbols_received) {

    ESP_LOGI("RMT", "INICIANDO RECEPÇÃO");

    rmt_rx_disable();
    vTaskDelay(pdMS_TO_TICKS(10));

    esp_err_t err_receive = rmt_rx_start_receive(raw_symbols_receive, max_symbols);

    if (err_receive != ESP_OK) {
        ESP_LOGE("RMT", "Erro ao iniciar recepção: %s", esp_err_to_name(err_receive));
        return false;
    }

    ESP_LOGI("RMT", "AGUARDANDO SINAL IR...");

    rmt_rx_done_event_data_t rx_data;
    if (xQueueReceive(receive_queue, &rx_data, pdMS_TO_TICKS(10000)) == pdTRUE) {  // Alterado para 10 segundos
        size_t n = rx_data.num_symbols;

        if (symbols_received) *symbols_received = n;

        // Cópia segura para raw_symbols_receive
        if (n > max_symbols) n = max_symbols;
        memcpy(raw_symbols_receive, rx_data.received_symbols, n * sizeof(rmt_symbol_word_t));

        ESP_LOGI("RMT", "✓ Sinal recebido com %d símbolos", (int)n);
        return true;
    }

    ESP_LOGW("RMT", "⏱ Timeout de recepção");
    rmt_rx_receive_stop_queue();

    return false;
}