// Ficheiro: backlight.c

#include "backlight.h"
#include "driver/ledc.h"
#include "pin_def.h"
#include "st7789.h" // Assumindo que ST7789_PIN_BL está definido aqui

// --- Configurações do Periférico LEDC (PWM) ---
#define LEDC_TIMER              LEDC_TIMER_1
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_1
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT // Resolução de 8 bits (permite valores de 0 a 255)
#define LEDC_FREQUENCY          (5000)           // Frequência de 5 kHz para o PWM

// Variável estática para guardar o brilho atual.
// 'static' significa que esta variável só é visível dentro deste ficheiro.
static uint8_t g_brightness;

void backlight_init(void)
{
    // 1. Configurar o Timer do LEDC
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // 2. Configurar o Canal do LEDC
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = ST7789_PIN_BL, // O pino do backlight é atribuído aqui
        .duty           = 255, // Começa diretamente com o brilho no máximo
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);

    // 3. Sincronizar a variável de estado com o valor inicial do hardware
    g_brightness = 255;
}

void backlight_set_brightness(uint8_t brightness)
{
    // Guarda o novo valor de brilho na nossa variável de estado
    g_brightness = brightness;
    
    // Define o novo duty cycle (brilho) para o canal LEDC
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, g_brightness);
    
    // Aplica a mudança de brilho no hardware
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

uint8_t backlight_get_brightness(void)
{
    // Simplesmente retorna o valor atual que está guardado na nossa variável
    return g_brightness;
}