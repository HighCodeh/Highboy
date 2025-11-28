// Copyright (c) 2025 HIGH CODE LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "cc1101.h"
#include "spi.h"
#include "pin_def.h"
#include <string.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/task.h>
#include <rom/ets_sys.h>

static const char *TAG = "CC1101";
static spi_device_handle_t cc1101_spi;

// Função auxiliar: Envia um strobe (comando) ao CC1101 via SPI
void cc1101_strobe(uint8_t cmd)
{
    spi_transaction_t t = {
        .length = 8,
        .flags = SPI_TRANS_USE_TXDATA
    };
    t.tx_data[0] = cmd;
    spi_device_transmit(cc1101_spi, &t);
}

// Função auxiliar: Escreve um valor em um registrador do CC1101
void cc1101_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t tx_data[2] = {reg, val};
    spi_transaction_t t = {0};
    t.length = 8 * 2;
    t.tx_buffer = tx_data;
    spi_device_transmit(cc1101_spi, &t);
}

// Função auxiliar: Lê um valor de um registrador do CC1101
uint8_t cc1101_read_reg(uint8_t reg)
{
    uint8_t tx_data[2] = {0x80 | reg, 0};
    uint8_t rx_data[2] = {0};
    spi_transaction_t t = {0};
    t.length = 8 * 2;
    t.tx_buffer = tx_data;
    t.rx_buffer = rx_data;
    spi_device_transmit(cc1101_spi, &t);
    return rx_data[1];
}

// Função auxiliar: Escreve múltiplos bytes em modo burst
void cc1101_write_burst(uint8_t reg, const uint8_t *buf, uint8_t len)
{
    uint8_t addr = 0x40 | reg; // Burst write flag = 0x40
    spi_transaction_t t = {0};
    t.length = 8 * (len + 1);
    uint8_t *data = malloc(len + 1);
    data[0] = addr;
    memcpy(&data[1], buf, len);
    t.tx_buffer = data;
    spi_device_transmit(cc1101_spi, &t);
    free(data);
}

// Inicializa CC1101 usando o driver SPI centralizado
void cc1101_init(void)
{
    // ========== ADICIONA CC1101 NO DRIVER SPI ==========
    spi_device_config_t cc1101_cfg = {
        .cs_pin = CC1101_CS_PIN,
        .clock_speed_hz = 2 * 1000 * 1000,  // 2 MHz
        .mode = 0,                           // SPI mode 0
        .queue_size = 7,
    };
    
    esp_err_t ret = spi_add_device(SPI_DEVICE_CC1101, &cc1101_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao adicionar CC1101 no SPI: %s", esp_err_to_name(ret));
        return;
    }
    
    // Obtém o handle do device
    cc1101_spi = spi_get_handle(SPI_DEVICE_CC1101);
    if (!cc1101_spi) {
        ESP_LOGE(TAG, "Falha ao obter handle do CC1101");
        return;
    }
    // ===================================================

    // Configura GDO0 como entrada
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << CC1101_GDO0_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Reset do CC1101 via comando SRES
    cc1101_strobe(CC1101_SRES);
    esp_rom_delay_us(100);
    ESP_LOGI(TAG, "CC1101 Reset via SRES");

    // Configuração dos registradores (433MHz, 250kbps, MSK)
    cc1101_write_reg(CC1101_FSCTRL1, 0x0B);
    cc1101_write_reg(CC1101_FSCTRL0, 0x00);
    cc1101_write_reg(CC1101_FREQ2,   0x10);
    cc1101_write_reg(CC1101_FREQ1,   0xA7);
    cc1101_write_reg(CC1101_FREQ0,   0x62);

    cc1101_write_reg(CC1101_MDMCFG4, 0x2D);
    cc1101_write_reg(CC1101_MDMCFG3, 0x3B);
    cc1101_write_reg(CC1101_MDMCFG2, 0x73);
    cc1101_write_reg(CC1101_MDMCFG1, 0x22);
    cc1101_write_reg(CC1101_MDMCFG0, 0xF8);
    cc1101_write_reg(CC1101_CHANNR,  0x00);

    cc1101_write_reg(CC1101_MCSM0,   0x18);
    cc1101_write_reg(CC1101_DEVIATN, 0x00);
    cc1101_write_reg(CC1101_FREND1,  0xB6);
    cc1101_write_reg(CC1101_FREND0,  0x10);

    cc1101_write_reg(CC1101_FOCCFG,   0x1D);
    cc1101_write_reg(CC1101_BSCFG,    0x1C);
    cc1101_write_reg(CC1101_AGCCTRL2, 0xC7);
    cc1101_write_reg(CC1101_AGCCTRL1, 0x00);
    cc1101_write_reg(CC1101_AGCCTRL0, 0xB2);

    cc1101_write_reg(CC1101_FSCAL3, 0xEA);
    cc1101_write_reg(CC1101_FSCAL2, 0x0A);
    cc1101_write_reg(CC1101_FSCAL1, 0x00);
    cc1101_write_reg(CC1101_FSCAL0, 0x11);

    cc1101_write_reg(CC1101_FSTEST, 0x59);
    cc1101_write_reg(CC1101_TEST2,  0x88);
    cc1101_write_reg(CC1101_TEST1,  0x31);
    cc1101_write_reg(CC1101_TEST0,  0x0B);

    cc1101_write_reg(CC1101_IOCFG2,   0x0B);
    cc1101_write_reg(CC1101_IOCFG0,   0x06);
    cc1101_write_reg(CC1101_PKTCTRL1, 0x04);
    cc1101_write_reg(CC1101_PKTCTRL0, 0x05);
    cc1101_write_reg(CC1101_ADDR,     0x00);
    cc1101_write_reg(CC1101_PKTLEN,   0xFF);

    cc1101_write_reg(CC1101_PATABLE, 0x50);

    // Limpa FIFOs
    cc1101_strobe(CC1101_SFRX);
    cc1101_strobe(CC1101_SFTX);

    ESP_LOGI(TAG, "CC1101 inicializado (433MHz, 250kbps, MSK)");
}

// Envia um pacote de dados
void cc1101_send_data(const uint8_t *data, size_t len)
{
    cc1101_write_reg(CC1101_TXFIFO, (uint8_t)len);
    cc1101_write_burst(CC1101_TXFIFO, data, len);
    cc1101_strobe(CC1101_STX);
    ESP_LOGI(TAG, "STX enviado, aguardando GDO0...");

    while (gpio_get_level(CC1101_GDO0_PIN) == 0) {
        // Aguarda GDO0 subir
    }
    ESP_LOGI(TAG, "GDO0 subiu - TX iniciado");

    while (gpio_get_level(CC1101_GDO0_PIN) == 1) {
        // Aguarda GDO0 cair
    }
    ESP_LOGI(TAG, "GDO0 caiu - TX finalizado");

    cc1101_strobe(CC1101_SFTX);
}

// Entra em modo de recepção contínua
void cc1101_enter_receive(void)
{
    cc1101_strobe(CC1101_SRX);
    ESP_LOGI(TAG, "CC1101 em modo RX");
}
