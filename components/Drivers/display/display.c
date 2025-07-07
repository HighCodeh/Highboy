#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/spi_master.h"
#include "st7789.h"
#include "pin_def.h" 
#include "esp_log.h"

void init_display(void) {
    spi_bus_config_t buscfg = {
        .mosi_io_num = ST7789_PIN_MOSI,
        .sclk_io_num = ST7789_PIN_SCLK,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        // ✨ ALTERADO: Aumenta o tamanho máximo para a transferência do framebuffer inteiro
        .max_transfer_sz = ST7789_WIDTH * ST7789_HEIGHT * 2 + 8,
    };

    esp_err_t ret = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        printf("Erro ao inicializar SPI: %s\n", esp_err_to_name(ret));
        return;
    }

    // A função st7789_init() agora faz tudo:
    // - Inicializa o display
    // - Aloca o framebuffer
    // - Limpa a tela com preto
    // - Atualiza a tela (flush)
    // - Liga o backlight
    st7789_init();

    // ✨ REMOVIDO: As linhas abaixo não são mais necessárias.
    // st7789_enable_framebuffer();
    // st7789_set_backlight(true);
    // st7789_fill_screen(ST7789_COLOR_BLACK);
    
    // Você pode manter esta linha se quiser que o tamanho padrão do texto seja 2.
    st7789_set_text_size(2);
}
