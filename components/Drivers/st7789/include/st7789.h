#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/spi_master.h"

// ** Configurações do display ST7789 **
#define ST7789_WIDTH    240   // Largura do display em pixels
#define ST7789_HEIGHT   240   // Altura do display em pixels

// Offsets de memória (ajuste para seu módulo, se necessário)
#define ST7789_X_OFFSET 0     // Offset horizontal (ex: 0 ou 80)
#define ST7789_Y_OFFSET 0     // Offset vertical   (ex: 0 ou 80)

// ** Pinos da interface SPI3 (VSPI) no ESP32-S3 **
#define ST7789_PIN_MOSI 11    // GPIO11 para MOSI (Master-Out, Slave-In)
#define ST7789_PIN_SCLK 12    // GPIO12 para SCLK (clock SPI)
#define ST7789_PIN_CS   48   // GPIO47 para Chip Select (CS)
#define ST7789_PIN_DC   47    // GPIO48 para Data/Command (DC)
#define ST7789_PIN_RST  21    // GPIO21 para Reset do display
#define ST7789_PIN_BL   38    // GPIO38 para Backlight (BL) do display

// ** Cores em formato RGB565 (5 bits R, 6 bits G, 5 bits B) **
#define ST7789_COLOR_BLACK       0x0000  // R=0,   G=0,   B=0
#define ST7789_COLOR_WHITE       0xFFFF  // R=255, G=255, B=255
#define ST7789_COLOR_RED         0xF800  // R=255, G=0,   B=0
#define ST7789_COLOR_GREEN       0x07E0  // R=0,   G=255, B=0
#define ST7789_COLOR_BLUE        0x001F  // R=0,   G=0,   B=255
#define ST7789_COLOR_YELLOW      0xFFE0  // R=255, G=255, B=0
#define ST7789_COLOR_CYAN        0x07FF  // R=0,   G=255, B=255
#define ST7789_COLOR_MAGENTA     0xF81F  // R=255, G=0,   B=255

#define ST7789_COLOR_DARKPURPLE  0x4013  // Darker purple for contrast
#define ST7789_COLOR_LIGHTPURPLE 0x915F  // Lighter purple for highlights
#define ST7789_COLOR_DARKGRAY    0x4208  // Dark gray for inactive elements
#define ST7789_COLOR_GRAY        0x8410  // Medium gray
 

// Tons de roxo e outras cores estendidas (em RGB565)
#define ST7789_COLOR_DARK_PURPLE 0x380A  // Roxo escuro
#define ST7789_COLOR_PURPLE      (ST7789_COLOR565(122, 5, 255))   // Roxo médio (~0x895F)
#define ST7789_COLOR_LIGHT_PURPLE 0xB419 // Roxo claro
#define ST7789_COLOR_VIOLET      0xEC1D  // Violeta
#define ST7789_COLOR_LAVENDER    0xE73F  // Lavanda
#define ST7789_COLOR_ORCHID      0xDB9A  // Orquídea
#define ST7789_COLOR_AMETHYST    0x9B39  // Ametista
#define ST7789_COLOR_INDIGO      0x4810  // Índigo (R=75, G=0, B=130)

// Macro para converter valores 24-bit RGB para RGB565 com arredondamento
#define ST7789_COLOR565(r, g, b) (                                       \
    ((((r) * 249 + 1014) >> 11) & 0x1F) << 11 |                          \
    ((((g) * 253 + 505) >> 10) & 0x3F) << 5  |                          \
    ((((b) * 249 + 1014) >> 11) & 0x1F)                                  \
)

// Limite de bytes por transferência SPI (divide envios grandes para DMA)
#define ST7789_MAX_CHUNK_BYTES 4096
#define SWAP(a, b) { int16_t t = a; a = b; b = t; }

// Dados de exemplo de ícones (monocromáticos 1-bit por pixel)
extern const uint8_t octo[];
extern const uint8_t octor[];
extern const uint8_t wifi[];
extern const uint8_t ble[];
extern const uint8_t ant[];
extern const uint8_t bat[];
extern const uint8_t image_wi_fi_bits[];
extern const uint8_t image_bluetooth_bits[];
extern const uint8_t image_nfc_bits[];
extern const uint8_t image_infravermelho_bits[];

uint16_t *st7789_get_framebuffer(void);
void st7789_update(void);



// Ícone do mascote Octo


// ** Protótipos das funções do driver ST7789 **
void st7789_init(void);
// Adicione este protótipo na seção de funções do display
extern const uint8_t dvd_logo_48x24[];
void st7789_set_rotation(uint8_t rotation);  // 0, 1, 2 ou 3 (orientação da tela)
void st7789_draw_hline(int x, int y, int w, uint16_t color);
void st7789_set_backlight(bool on);
void st7789_fill_screen(uint16_t color);
void st7789_draw_pixel(int x, int y, uint16_t color);
void st7789_draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void st7789_draw_rect(int x, int y, int w, int h, uint16_t color);
void st7789_draw_round_rect(int x, int y, int w, int h, int r, uint16_t color);
void st7789_set_text_size(int size);
void st7789_draw_bitmapRGB(int x, int y, const uint16_t *bitmap, int w, int h);
void st7789_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void st7789_write_color(uint16_t color);

void st7789_fill_rect_fb(int x0, int y0, int w, int h, uint16_t color);

void st7789_draw_bitmap_1bit_fb(int x, int y, const uint8_t *bitmap, int w, int h, uint16_t fg, uint16_t bg);



void st7789_draw_filled_rect(int x, int y, int w, int h, uint16_t color);

void st7789_draw_text_centered(int centerX, int y, const char *text, uint16_t color);


void st7789_draw_bitmap(int x, int y, const uint8_t *bitmap, int w, int h, uint16_t color);
void st7789_draw_image(int x, int y, int w, int h, const uint16_t *image);
void st7789_draw_char(int x, int y, char c, uint16_t color);
void st7789_draw_text(int x, int y, const char *text, uint16_t color);
void st7789_draw_icon(int x, int y, const uint8_t *icon_bits, uint16_t fg_color, uint16_t bg_color);

void st7789_enable_framebuffer(void);
void st7789_draw_pixel_fb(int x, int y, uint16_t color);
void st7789_mark_dirty(int x, int y, int w, int h);
void st7789_update_dirty(void);

void st7789_draw_button(int x, int y, int w, int h, const char *label,
                        uint16_t text_color, uint16_t fill_color, uint16_t border_color);
void st7789_fill_round_rect(int x, int y, int w, int h, int r, uint16_t color);
void st7789_fill_circle(int x0, int y0, int r, uint16_t color);
void st7789_draw_circle(int x0, int y0, int radius, uint16_t color);

// Triangle functions
void st7789_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);
void st7789_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);

// Text functions
void st7789_draw_small_text(int x, int y, const char* text, uint16_t color);

void st7789_reset_dirty_rect(void);
void st7789_draw_char_fb(int x, int y, char c, uint16_t color, uint16_t bg_color);
void st7789_scroll_text(int x, int y, int offset_y, const char *text, uint16_t color, uint16_t bg_color);


//static void set_addr_window_direct(int x, int y, int w, int h);
void st7789_flush();
void st7789_draw_pixel_fb(int x, int y, uint16_t color);
void st7789_fill_screen_fb(uint16_t color);
void st7789_draw_hline_fb(int x, int y, int w, uint16_t color);
void st7789_draw_vline_fb(int x, int y, int h, uint16_t color);
void st7789_fill_rect_fb(int x, int y, int w, int h, uint16_t color);
void st7789_draw_rect_fb(int x, int y, int w, int h, uint16_t color);
void st7789_draw_char_fb(int x, int y, char c, uint16_t color, uint16_t bg_color);
void st7789_draw_text_fb(int x, int y, const char *text, uint16_t color, uint16_t bg_color);
void st7789_draw_image_fb(int x, int y, int w, int h, const uint16_t *image);
void st7789_draw_line_fb(int x0, int y0, int x1, int y1, uint16_t color);
void st7789_draw_circle_fb(int x0, int y0, int r, uint16_t color);
void st7789_fill_circle_fb(int x0, int y0, int r, uint16_t color);
void st7789_draw_round_rect_fb(int x, int y, int w, int h, int r, uint16_t color);
void st7789_draw_bitmap_fb(int x, int y, const uint8_t *bitmap, int w, int h, uint16_t color);
//static void draw_quarter_circle_fb(int x, int y, int r, int corner, uint16_t color);
void st7789_fill_round_rect_fb(int x, int y, int w, int h, int r, uint16_t color);
//static void fill_circle_helper_fb(int16_t x0, int16_t y0, int16_t r, uint8_t corner, int16_t delta, uint16_t color);
// Adicione esta linha em st7789.h
uint16_t st7789_rgb_to_color(uint8_t r, uint8_t g, uint8_t b);

uint16_t* st7789_get_framebuffer();

#endif // ST7789_H
