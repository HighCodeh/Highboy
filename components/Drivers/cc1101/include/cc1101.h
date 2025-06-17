#ifndef CC1101_H
#define CC1101_H

#include <stdint.h>
#include <stddef.h>

// SPI handle (inicializado em cc1101_init)
#include <driver/spi_master.h>
extern spi_device_handle_t cc1101_spi;
#define CC1101_TXFIFO 0x3F
  #define CC1101_RXFIFO 0x3F

// Pinos (ajustar conforme hardware)
#define CC1101_SPI_HOST    SPI3_HOST    // SPI2_HOST (HSPI) ou SPI3_HOST (VSPI)
#define CC1101_CS_PIN      3       // Exemplo: GPIO15 como CS
#define CC1101_MOSI_PIN    11           // Exemplo: GPIO13 como MOSI
#define CC1101_MISO_PIN    13          // Exemplo: GPIO12 como MISO
#define CC1101_SCLK_PIN    12          // Exemplo: GPIO14 como SCLK
#define CC1101_GDO0_PIN    42         // Exemplo: GPIO25 como GDO0 (indicador de pacote)

// Definições de registradores (endereços do CC1101)
#define CC1101_IOCFG2    0x00  // GDO2 output pin config
#define CC1101_IOCFG0    0x06 // GDO0 output pin config
#define CC1101_FIFOTHR   0x03
#define CC1101_SYNC1     0x04
#define CC1101_SYNC0     0x05
#define CC1101_PKTLEN    0x06
#define CC1101_PKTCTRL1  0x07
#define CC1101_PKTCTRL0  0x08
#define CC1101_ADDR      0x09
#define CC1101_CHANNR    0x0A
#define CC1101_FSCTRL1   0x0B
#define CC1101_FSCTRL0   0x0C
#define CC1101_FREQ2     0x0D
#define CC1101_FREQ1     0x0E
#define CC1101_FREQ0     0x0F
#define CC1101_MDMCFG4   0x10
#define CC1101_MDMCFG3   0x11
#define CC1101_MDMCFG2   0x12
#define CC1101_MDMCFG1   0x13
#define CC1101_MDMCFG0   0x14
#define CC1101_DEVIATN   0x15
#define CC1101_MCSM0     0x18
#define CC1101_FOCCFG    0x19
#define CC1101_BSCFG     0x1A
#define CC1101_AGCCTRL2  0x1B
#define CC1101_AGCCTRL1  0x1C
#define CC1101_AGCCTRL0  0x1D
#define CC1101_FREND1    0x21
#define CC1101_FREND0    0x22
#define CC1101_FSCAL3    0x23
#define CC1101_FSCAL2    0x24
#define CC1101_FSCAL1    0x25
#define CC1101_FSCAL0    0x26
#define CC1101_FSTEST    0x29
#define CC1101_TEST2     0x2C
#define CC1101_TEST1     0x2D
#define CC1101_TEST0     0x2E
#define CC1101_PATABLE   0x3E  // PA Table

// Comandos de strobe do CC1101 (endereços 0x30-0x3D)
#define CC1101_SRES      0x30  // Reset chip
#define CC1101_SFSTXON   0x31  // Enable/calibrate freq synthesizer
#define CC1101_SXOFF     0x32  // Turn off crystal oscillator
#define CC1101_SCAL      0x33  // Calibrate freq synthesizer and turn it off
#define CC1101_SRX       0x34  // Enable RX
#define CC1101_STX       0x35  // Enable TX
#define CC1101_SIDLE     0x36  // Exit RX / TX, turn off synthesizer
#define CC1101_SPWD      0x39  // Enter power-down mode
#define CC1101_SFRX      0x3A  // Flush RX FIFO
#define CC1101_SFTX      0x3B  // Flush TX FIFO
#define CC1101_SWORRST   0x3C  // Reset real time clock
#define CC1101_SNOP      0x3D  // No operation

// Prototipos das funcoes
void cc1101_init(void);
void cc1101_write_reg(uint8_t reg, uint8_t val);
uint8_t cc1101_read_reg(uint8_t reg);
void cc1101_write_burst(uint8_t reg, const uint8_t *buf, uint8_t len);
void cc1101_strobe(uint8_t cmd);
void cc1101_send_data(const uint8_t *data, size_t len);
void cc1101_enter_receive(void);

#endif // CC1101_H
