#include "cc1101.h"
#include <string.h>
#include <esp_log.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <freertos/task.h>


static const char *TAG = "CC1101";
spi_device_handle_t cc1101_spi;  // Handle SPI do dispositivo CC1101
#define RX_MAX_PACKET_SIZE 64
// Função auxiliar: Envia um strobe (comando) ao CC1101 via SPI
void cc1101_strobe(uint8_t cmd)
{
    spi_transaction_t t = {
        .length = 8,       // 1 byte
        .tx_buffer = &cmd,
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

// Função auxiliar: Escreve múltiplos bytes em modo burst (FIFO ou PATABLE, etc.)
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

// Inicializa SPI e configura CC1101 com os registradores adequados
void cc1101_init(void)
{
    esp_err_t ret;

    // 1) Configura pinos GPIO para SPI
    spi_bus_config_t buscfg = {
        .miso_io_num = CC1101_MISO_PIN,
        .mosi_io_num = CC1101_MOSI_PIN,
        .sclk_io_num = CC1101_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    ret = spi_bus_initialize(CC1101_SPI_HOST, &buscfg, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed");
        return;
    }

    // 2) Adiciona dispositivo SPI para o CC1101
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 2*1000*1000, // 2 MHz (exemplo)
        .mode = 0,                     // SPI mode 0
        .spics_io_num = CC1101_CS_PIN, // GPIO do CS
        .queue_size = 7,
        .flags = 0  // usa full-duplex, necessário para leitura burst

    };
    ret = spi_bus_add_device(CC1101_SPI_HOST, &devcfg, &cc1101_spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed");
        return;
    }

    // 3) Configura GDO0 como entrada para indicações do CC1101
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << CC1101_GDO0_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // 4) Reset do CC1101 via comando SRES
    cc1101_strobe(CC1101_SRES);
    esp_rom_delay_us(100);   // espera ~ >40µs (conforme datasheet):contentReference[oaicite:4]{index=4}
    ESP_LOGI(TAG, "CC1101 Reset via SRES");

    // 5) Escrita dos registradores de configuração (valores de RF_SETTINGS)
    // Baseado em exemplos de SmartRF Studio / datasheet para 433MHz, 250kbps MSK:contentReference[oaicite:5]{index=5}:contentReference[oaicite:6]{index=6}.

    // Frequência portadora: 433MHz (FREQ2,FREQ1,FREQ0 definem ~433.000 MHz)
    cc1101_write_reg(CC1101_FSCTRL1, 0x0B);  // IF = 152.343 kHz (freq synthesizer control):contentReference[oaicite:7]{index=7}
    cc1101_write_reg(CC1101_FSCTRL0, 0x00);
    cc1101_write_reg(CC1101_FREQ2,   0x10);
    cc1101_write_reg(CC1101_FREQ1,   0xA7);
    cc1101_write_reg(CC1101_FREQ0,   0x62);  // FREQ = 0x10A762 ≈ 432.9998 MHz:contentReference[oaicite:8]{index=8}:contentReference[oaicite:9]{index=9}

    // Modem: 250 kbps, modulação MSK
    cc1101_write_reg(CC1101_MDMCFG4, 0x2D); // Data rate exponent/mantissa
    cc1101_write_reg(CC1101_MDMCFG3, 0x3B); // Data rate mantissa
    cc1101_write_reg(CC1101_MDMCFG2, 0x73); // Demodulator = MSK (bits 6:4 = 111):contentReference[oaicite:10]{index=10}, digital filter off
    cc1101_write_reg(CC1101_MDMCFG1, 0x22); // Channel spacing = 199.951 kHz (mantissa)
    cc1101_write_reg(CC1101_MDMCFG0, 0xF8); // Channel spacing exponent
    cc1101_write_reg(CC1101_CHANNR,   0x00); // Canal 0

    // MCSM0:  estado pós pacote TX/RX
    cc1101_write_reg(CC1101_MCSM0,    0x18); // High byte of TXOFF_MODE and RXOFF_MODE

    // Deviation (para MSK não usado, mantido 0)
    cc1101_write_reg(CC1101_DEVIATN,  0x00); // Desvio nulo (MSK implícito)

    // Front-end setting (não usar PA para recepção)
    cc1101_write_reg(CC1101_FREND1,   0xB6); // Front end RX config
    cc1101_write_reg(CC1101_FREND0,   0x10); // Front end TX config

    // Configurações adicionais de sintonia e AGC
    cc1101_write_reg(CC1101_FOCCFG,   0x1D); // Frequency offset compensation
    cc1101_write_reg(CC1101_BSCFG,    0x1C); // Bit synchronization
    cc1101_write_reg(CC1101_AGCCTRL2, 0xC7); // AGC control (gain control settings)
    cc1101_write_reg(CC1101_AGCCTRL1, 0x00);    
    cc1101_write_reg(CC1101_AGCCTRL0, 0xB2);

    // Calibração do sintetizador de frequência
    cc1101_write_reg(CC1101_FSCAL3,   0xEA);
    cc1101_write_reg(CC1101_FSCAL2,   0x0A);
    cc1101_write_reg(CC1101_FSCAL1,   0x00);
    cc1101_write_reg(CC1101_FSCAL0,   0x11);

    // Configurações de teste (valores padrão otimizados)
    cc1101_write_reg(CC1101_FSTEST,   0x59);
    cc1101_write_reg(CC1101_TEST2,    0x88);
    cc1101_write_reg(CC1101_TEST1,    0x31);
    cc1101_write_reg(CC1101_TEST0,    0x0B);

    // Configuração de pinos de uso geral (GDO) e pacote
    cc1101_write_reg(CC1101_IOCFG2,   0x0B); // GDO2 = Serial clock (CLK_XOSC/??)
    cc1101_write_reg(CC1101_IOCFG0,   0x06); // GDO0 = Asserto quando SYNC word enviado (TX/RX):contentReference[oaicite:11]{index=11}
    cc1101_write_reg(CC1101_PKTCTRL1, 0x04); // Habilita CRC, no pad
    cc1101_write_reg(CC1101_PKTCTRL0, 0x05); // Formato fixo, CRC habilitado, limpeza automática do FIFO
    cc1101_write_reg(CC1101_ADDR,     0x00); // Endereço do dispositivo (não usado)
    cc1101_write_reg(CC1101_PKTLEN,   0xFF); // Comprimento de pacote máximo (modo fixo)

    // Ajusta PA Table para 0 dBm (primeiro índice = 0x50):contentReference[oaicite:12]{index=12}  
    cc1101_write_reg(CC1101_PATABLE,  0x50);

    // Limpa FIFOs por garantia
    cc1101_strobe(CC1101_SFRX);
    cc1101_strobe(CC1101_SFTX);

    ESP_LOGI(TAG, "CC1101 configured (433MHz, 250kbps, MSK)");
}

// Envia um pacote de dados (modo FIFO baseado em PKTLEN=0xFF fixo)
// data = ponteiro para dados, len = número de bytes (até 255)
void cc1101_send_data(const uint8_t *data, size_t len)
{
    cc1101_write_reg(CC1101_TXFIFO, (uint8_t)len);
    cc1101_write_burst(CC1101_TXFIFO, data, len);
    cc1101_strobe(CC1101_STX);
    ESP_LOGI(TAG, "STX enviado, aguardando GDO0...");

    while (gpio_get_level(CC1101_GDO0_PIN) == 0) {
        // Esperando GDO0 subir
    }
    ESP_LOGI(TAG, "GDO0 subiu - TX iniciado");

    while (gpio_get_level(CC1101_GDO0_PIN) == 1) {
        // Esperando GDO0 cair
    }
    ESP_LOGI(TAG, "GDO0 caiu - TX finalizado");

    cc1101_strobe(CC1101_SFTX); // Limpa FIFO TX
}


// Entra em modo de recepção contínua
void cc1101_enter_receive(void)
{
    cc1101_strobe(CC1101_SRX); // Comando SRX habilita RX (calibra se necessário)
    ESP_LOGI(TAG, "CC1101 entered RX mode");
}
