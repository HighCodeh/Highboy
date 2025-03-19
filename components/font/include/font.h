// fonts.h
#pragma once
#include <stdint.h>

typedef struct {
    const uint8_t *bitmap;     // Dados dos glifos (bitmap contíguo)
    uint16_t first_char;       // Primeiro caractere
    uint16_t last_char;        // Último caractere
    uint8_t width;             // Largura do glifo
    uint8_t height;            // Altura do glifo
    uint8_t spacing;           // Espaçamento entre caracteres
} font_t;

// Fontes disponíveis
extern const font_t font_ubuntu_16;
extern const font_t font_terminus_24;
extern const font_t font_arial_32;