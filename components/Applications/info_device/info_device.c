// Arquivo: info_device.c
#include "info_device.h"
#include "st7789.h"
#include "pin_def.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

// Includes para buscar informações do sistema
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"

// Cores do tema
#define THEME_COLOR ST7789_COLOR_PURPLE
#define TEXT_COLOR ST7789_COLOR_WHITE
#define BG_COLOR ST7789_COLOR_BLACK

// Função auxiliar para desenhar uma linha de informação
static void draw_info_line(int y, const char* label, const char* value) {
    st7789_draw_text_fb(15, y, label, TEXT_COLOR, BG_COLOR);
    st7789_draw_text_fb(120, y, value, TEXT_COLOR, BG_COLOR);
}

// Função principal que desenha e controla a tela
void show_device_info_screen(void) {
    // --- 1. Coleta de Dados do Sistema ---

    // Informações do Chip
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    // Informações da Memória Flash
    uint32_t flash_size_mb = 0;
    uint32_t flash_size;
    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        flash_size_mb = flash_size / (1024 * 1024);
    }
    
    // Informações da Memória RAM (Heap)
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);

    // Buffers para formatar os dados em texto
    char model_str[20];
    char info_str[40];

    // --- 2. Desenho da Interface ---

    st7789_fill_screen_fb(BG_COLOR);
    st7789_set_text_size(2); // Garante o tamanho padrão para esta tela

    // Título
    st7789_draw_text_fb(15, 15, "Info do Dispositivo", THEME_COLOR, BG_COLOR);
    st7789_draw_hline_fb(10, 35, 220, THEME_COLOR);

    // Seção de Hardware
    snprintf(model_str, sizeof(model_str), "ESP32-S%d", (chip_info.model == CHIP_ESP32S3 ? 3 : chip_info.model));
    draw_info_line(50, "Modelo:", model_str);

    snprintf(info_str, sizeof(info_str), "Rev.%d, %d Cores", chip_info.revision, chip_info.cores);
    draw_info_line(70, "Chip:", info_str);

    snprintf(info_str, sizeof(info_str), "%lu MB", flash_size_mb);
    draw_info_line(90, "Flash:", info_str);

    // Seção de Memória
    st7789_draw_hline_fb(10, 115, 220, THEME_COLOR);

    snprintf(info_str, sizeof(info_str), "%d KB", (int)(total_heap / 1024));
    draw_info_line(130, "RAM Total:", info_str);

    snprintf(info_str, sizeof(info_str), "%d KB Livres", (int)(free_heap / 1024));
    draw_info_line(150, "RAM Livre:", info_str);

    // Seção de Firmware
    st7789_draw_hline_fb(10, 175, 220, THEME_COLOR);
    draw_info_line(190, "Firmware:", esp_get_idf_version());
    
    // Atualiza a tela com tudo que foi desenhado
    st7789_flush();

    // --- 3. Loop de Espera ---
    // Espera o botão VOLTAR ser pressionado para sair
    while (gpio_get_level(BTN_OK) == 0) { vTaskDelay(pdMS_TO_TICKS(10)); } // Evita sair se OK foi usado para entrar
    
    while(1) {
        if (gpio_get_level(BTN_BACK) == 0) {
            vTaskDelay(pdMS_TO_TICKS(200)); // Debounce
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}