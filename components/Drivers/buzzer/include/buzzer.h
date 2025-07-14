// Arquivo: buzzer.h

#ifndef BUZZER_H
#define BUZZER_H

#include "esp_err.h"
#include <stdint.h>


// --- DEFINIÇÕES DE HARDWARE PARA O BUZZER ---
// Para evitar conflitos, usamos um timer e canal diferente do backlight
#define BUZZER_LEDC_TIMER           LEDC_TIMER_1
#define BUZZER_LEDC_CHANNEL         LEDC_CHANNEL_1
#define BUZZER_LEDC_MODE            LEDC_LOW_SPEED_MODE
#define BUZZER_LEDC_DUTY_RESOLUTION LEDC_TIMER_13_BIT // Resolução para volume/duty
#define BUZZER_GPIO                 GPIO_NUM_46 // Confirme se este é o pino correto


#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978

esp_err_t buzzer_init(void);
void buzzer_play_tone(uint32_t freq_hz, uint32_t duration_ms);

// NOVA FUNÇÃO: Toca um som contínuo com volume variável
void buzzer_set_continuous_tone(uint8_t volume, uint32_t freq_hz);

// NOVA FUNÇÃO: Para o som contínuo
void buzzer_stop(void);
void buzzer_beep(void);
void buzzer_error(void);
void buzzer_click(void);
void buzzer_success(void);
void buzzer_play_mario_theme(void);
void buzzer_play_zeldas_lullaby(void);
void buzzer_hacker_glitch(void);
void buzzer_notify_short(void);
void buzzer_notify_long(void);
void buzzer_alarm(void);
void buzzer_critical_alert(void);
void buzzer_radar_ping(void);
void buzzer_digital_pulse(void);
void buzzer_access_granted(void);
void buzzer_access_denied(void);
void buzzer_scanner_loop(void);
void buzzer_boot_sequence(void);
void buzzer_system_tick(void);
void buzzer_scroll_tick(void);
void buzzer_hacker_confirm(void);
void buzzer_flipper_granted(void);
void buzzer_flipper_denied(void);
void buzzer_start_tone(uint32_t freq_hz, uint8_t volume);
void buzzer_stop_tone(void);


#endif // BUZZER_H
