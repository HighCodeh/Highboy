#include "ir_transmitter.h"
#include "driver/rmt_tx.h"  // Novo driver para transmissão
#include "driver/rmt_rx.h"  // Novo driver para recepção
#include "soc/rtc.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"  // Biblioteca para fila no FreeRTOS

rmt_channel_handle_t tx_chan = NULL;
static bool tx_chan_enabled = false;

//Callback do RX
bool on_trans_done(rmt_channel_handle_t chan, const rmt_tx_done_event_data_t *edata, void *user_data) {
    
    //ESP_LOGI("RMT", "⚡ Transmissão finalizada (callback).");
    return false;  

}

void rmt_tx_enable(){
    if (!tx_chan_enabled) {
        ESP_ERROR_CHECK(rmt_enable(tx_chan));
        tx_chan_enabled = true;
        ESP_LOGI("RMT", "Canal TX habilitado.");
    }
}

void rmt_tx_disable(){

    if(tx_chan_enabled){
        ESP_ERROR_CHECK(rmt_disable(tx_chan));
        tx_chan_enabled = false;
        ESP_LOGI("RMT", "Canal TX desabilitado.");
    }

} 

bool rmt_tx_delete(){

    if(tx_chan_enabled){
        ESP_ERROR_CHECK(rmt_disable(tx_chan));
    }

    esp_err_t err_tx_del = rmt_del_channel(tx_chan);
    if (err_tx_del != ESP_OK) {
        ESP_LOGE("RMT", "Falha ao desalocar canal TX: %s", esp_err_to_name(err_tx_del));
        return false;
    }
    
    ESP_LOGI("RMT", "Canal TX desalocado com sucesso.");
    return true;

}

bool rmt_tx_delete_encoder(rmt_encoder_handle_t encoder){

    esp_err_t err_del_encoder = rmt_del_encoder(encoder);
    if (err_del_encoder != ESP_OK) {
        ESP_LOGE("RMT", "Falha ao deletar encoder: %s", esp_err_to_name(err_del_encoder));
        return false;
    }
    
    ESP_LOGI("RMT", "Encoder deletado com sucesso.");
    return true;

}

bool rmt_tx_init(){

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,            // Fonte de clock padrão
        .gpio_num = TX_GPIO_NUM,                            // Usando o GPIO 18 para o transmissor IR
        .mem_block_symbols = MEM_BLOCK_SYMBOLS,                   // Tamanho do bloco de memória (64 símbolos)
        .resolution_hz = RESOLUTION_HZ,          // Resolução de 1 MHz (1 tick = 1 µs)
        .trans_queue_depth = TRANS_QUEUE_DEPTH,                    // Profundidade da fila de transações
        .flags.invert_out = FLAGS_INVERT_OUT,                 // Não inverter o sinal
        .flags.with_dma = FLAGS_WITH_DMA,                   // Não usar DMA
    };

    esp_err_t err_tx_channel = rmt_new_tx_channel(&tx_chan_config, &tx_chan);
    if (err_tx_channel != ESP_OK) {
        ESP_LOGE("RMT", "Falha ao criar canal TX: %s", esp_err_to_name(err_tx_channel));
        return false;;
    }
    ESP_LOGI("RMT", "Canal TX configurado com sucesso.");

    rmt_carrier_config_t tx_carrier_cfg = {
        .duty_cycle = DUTY_CYCLE,                 // Ciclo de trabalho de 33%
        .frequency_hz = FREQUENCY_HZ,              // Frequência de 38 KHz (frequência típica para IR)
        .flags.polarity_active_low = FLAGS_POLARITY_ACTIVE_LOW, // A portadora deve ser modulada para o nível alto
    };

    // Modulação da portadora no canal TX
    esp_err_t err_tx_carrier = rmt_apply_carrier(tx_chan, &tx_carrier_cfg);
    if (err_tx_carrier != ESP_OK) {
        ESP_LOGE("RMT", "Falha ao aplicar modulação de portadora no TX: %s", esp_err_to_name(err_tx_carrier));
        return false;
    }
    ESP_LOGI("RMT", "Modulação de portadora aplicada no canal TX.");

    // Registrando o callback no canal TX
    rmt_tx_event_callbacks_t tx_callbacks = {
        .on_trans_done = on_trans_done,  // Registra o callback para o evento de transmissão concluída
    };

    esp_err_t err_tx_callbacks = rmt_tx_register_event_callbacks(tx_chan, &tx_callbacks, NULL);
    if (err_tx_callbacks != ESP_OK) {
        ESP_LOGE("RMT", "Falha ao registrar callbacks do canal TX: %s", esp_err_to_name(err_tx_callbacks));
        return false;
    }

    return true;

}

bool rmt_tx_transmit(rmt_encoder_handle_t encoder, const rmt_symbol_word_t *data, size_t data_size){

    rmt_tx_enable();

    ESP_LOGI("RMT", "Iniciando transmissão...");

    rmt_transmit_config_t tx_config = {
        .loop_count = LOOP_COUNT,
    };
    
    // Iniciar a transmissão
    esp_err_t err_transmit = rmt_transmit(tx_chan, encoder, data, data_size, &tx_config);
    if (err_transmit != ESP_OK) {
        ESP_LOGE("RMT", "Falha ao iniciar transmissão: %s", esp_err_to_name(err_transmit));
        return false;
    }
    
    ESP_LOGI("RMT", "Transmissão iniciada com sucesso.");
    

    esp_err_t err_wait = rmt_tx_wait_all_done(tx_chan, 1000); // Timeout de 1 segundo
    if (err_wait == ESP_OK) {
        ESP_LOGI("RMT", "Todas as transmissões foram concluídas.");
    } else if (err_wait == ESP_ERR_TIMEOUT) {
        ESP_LOGW("RMT", "Timeout aguardando conclusão das transmissões.");
    } else {
        ESP_LOGE("RMT", "Erro aguardando transmissões: %s", esp_err_to_name(err_wait));
    }
    
    //rmt_tx_delete_encoder(encoder);
    return true;

}