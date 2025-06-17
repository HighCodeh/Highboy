#include "ir_transmitter.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

#define IR_GPIO 2                   // Pino do IR
#define CARRIER_FREQUENCY 38000     // 38kHz
#define DUTY_CYCLE 33               // 33% duty cycle

#define NEC_HDR_MARK 9000
#define NEC_HDR_SPACE 4500
#define NEC_BIT_MARK 560
#define NEC_ONE_SPACE 1690
#define NEC_ZERO_SPACE 560
#define NEC_END_MARK 560

static const char *TAG = "IR";

// === Gera o pulso de 38kHz por software ===
static void send_carrier(int time_us) {
    int period = 1000000 / CARRIER_FREQUENCY;
    int t1 = (period * DUTY_CYCLE) / 100;
    int t0 = period - t1;
    int cycles = time_us / period;

    for (int i = 0; i < cycles; i++) {
        gpio_set_level(IR_GPIO, 1);
        esp_rom_delay_us(t1);
        gpio_set_level(IR_GPIO, 0);
        esp_rom_delay_us(t0);
    }
}

// === Espaço (sem portadora) ===
static void send_space(int time_us) {
    gpio_set_level(IR_GPIO, 0);
    esp_rom_delay_us(time_us);
}

void ir_init(void) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << IR_GPIO),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(IR_GPIO, 0);
    ESP_LOGI(TAG, "IR Transmitter initialized on GPIO %d", IR_GPIO);
}

void ir_send_nec_raw(uint16_t address, uint8_t command) {
    uint8_t command_inv = ~command;
    uint16_t address_inv = ~address;

    uint64_t data = ((uint64_t)address) |
                    ((uint64_t)address_inv << 16) |
                    ((uint64_t)command << 32) |
                    ((uint64_t)command_inv << 40);

    // Header
    send_carrier(NEC_HDR_MARK);
    send_space(NEC_HDR_SPACE);

    // Envio dos 32 bits
    for (int i = 0; i < 32; i++) {
        send_carrier(NEC_BIT_MARK);
        if (data & (1ULL << i)) {
            send_space(NEC_ONE_SPACE);
        } else {
            send_space(NEC_ZERO_SPACE);
        }
    }

    // Stop bit
    send_carrier(NEC_END_MARK);
    send_space(0);
}
