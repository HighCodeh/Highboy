#include "buzzer.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Inicializa o LEDC para controle do buzzer
esp_err_t buzzer_init(void) {
    ledc_timer_config_t timer_conf = {
        .duty_resolution = LEDC_DUTY_RESOLUTION, // 13 bits
        .freq_hz = LEDC_FREQ,                    // freq inicial (pode ser alterada dinamicamente)
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .clk_cfg = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) {
        return err;
    }
    ledc_channel_config_t ch_conf = {
        .channel    = LEDC_CHANNEL,
        .duty       = 0,
        .gpio_num   = BUZZER_GPIO,
        .speed_mode = LEDC_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER
    };
    err = ledc_channel_config(&ch_conf);
    return err;
}

// Toca um tom na frequência (Hz) por duração (ms)
void buzzer_play_tone(uint32_t freq_hz, uint32_t duration_ms) {
    if (freq_hz > 0) {
        // Ajusta a frequência do timer LEDC e aplica 50% de duty
        ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq_hz);
        uint32_t duty = (1 << (LEDC_DUTY_RESOLUTION - 1)); // duty = metade do máximo
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    }
    // Toca pelo tempo especificado
    vTaskDelay(duration_ms / portTICK_PERIOD_MS);
    // Pára o tom (duty = 0 -> silêncio)
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

// Efeito: beep simples
void buzzer_beep(void) {
    buzzer_play_tone(NOTE_B5, 100);
}

// Efeito: sinal de erro (dois tons descendentes)
void buzzer_error(void) {
    buzzer_play_tone(NOTE_E5, 150);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    buzzer_play_tone(NOTE_C5, 150);
}

// Efeito: clique rápido
void buzzer_click(void) {
    buzzer_play_tone(NOTE_G5, 50);
}

// Efeito: sinal de sucesso (sequência ascendente)
void buzzer_success(void) {
    buzzer_play_tone(NOTE_C5, 100);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    buzzer_play_tone(NOTE_D5, 100);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    buzzer_play_tone(NOTE_E5, 100);
}
// Efeito: Hacker Glitch (trêmulo e agudo)
void buzzer_hacker_glitch(void) {
    for (int i = 0; i < 5; i++) {
        buzzer_play_tone(NOTE_DS6, 30);
        vTaskDelay(pdMS_TO_TICKS(10));
        buzzer_play_tone(NOTE_GS6, 30);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Efeito: Notificação curta
void buzzer_notify_short(void) {
    buzzer_play_tone(NOTE_G5, 80);
    vTaskDelay(pdMS_TO_TICKS(50));
    buzzer_play_tone(NOTE_C6, 100);
}

// Efeito: Notificação longa
void buzzer_notify_long(void) {
    buzzer_play_tone(NOTE_E5, 150);
    vTaskDelay(pdMS_TO_TICKS(50));
    buzzer_play_tone(NOTE_G5, 150);
    vTaskDelay(pdMS_TO_TICKS(50));
    buzzer_play_tone(NOTE_C6, 300);
}

// Efeito: Alarme (oscilante)
void buzzer_alarm(void) {
    for (int i = 0; i < 3; i++) {
        buzzer_play_tone(NOTE_C5, 250);
        buzzer_play_tone(NOTE_G4, 250);
    }
}

// Efeito: Alerta crítico (rápido e alto)
void buzzer_critical_alert(void) {
    for (int i = 0; i < 4; i++) {
        buzzer_play_tone(NOTE_B5, 100);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Efeito: Radar ping (eco sonoro)
void buzzer_radar_ping(void) {
    buzzer_play_tone(NOTE_G4, 100);
    vTaskDelay(pdMS_TO_TICKS(300));
    buzzer_play_tone(NOTE_E4, 80);
}

// Efeito: Digital pulse (tecnológico)
void buzzer_digital_pulse(void) {
    buzzer_play_tone(NOTE_C6, 40);
    vTaskDelay(pdMS_TO_TICKS(20));
    buzzer_play_tone(NOTE_E6, 40);
    vTaskDelay(pdMS_TO_TICKS(20));
    buzzer_play_tone(NOTE_G6, 40);
}

// Efeito: Acesso autorizado (positivo e rápido)
void buzzer_access_granted(void) {
    buzzer_play_tone(NOTE_A4, 100);
    vTaskDelay(pdMS_TO_TICKS(50));
    buzzer_play_tone(NOTE_C5, 120);
}

// Efeito: Acesso negado (tom de erro com fade)
void buzzer_access_denied(void) {
    buzzer_play_tone(NOTE_DS4, 200);
    buzzer_play_tone(NOTE_D4, 200);
    buzzer_play_tone(NOTE_CS4, 200);
}

// Efeito: Scanner looping (repetição digital)
void buzzer_scanner_loop(void) {
    for (int i = 0; i < 3; i++) {
        buzzer_play_tone(NOTE_C4 + (i * 50), 50);
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

// Efeito: Boot digital (estilo inicialização do sistema)
void buzzer_boot_sequence(void) {
    buzzer_play_tone(NOTE_C5, 80);
    vTaskDelay(pdMS_TO_TICKS(50));
    buzzer_play_tone(NOTE_E5, 60);
    vTaskDelay(pdMS_TO_TICKS(30));
    buzzer_play_tone(NOTE_G5, 100);
}

// Efeito: System tick (pulsos rápidos de sistema)
void buzzer_system_tick(void) {
    for (int i = 0; i < 3; i++) {
        buzzer_play_tone(NOTE_C6, 25);
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

// Efeito: Menu scroll (scrolling sonoro)
void buzzer_scroll_tick(void) {
    buzzer_play_tone(NOTE_C6, 20);
    vTaskDelay(pdMS_TO_TICKS(20));
    buzzer_play_tone(NOTE_E6, 20);
}

// Efeito: Hacker confirm (tom sintético de confirmação)
void buzzer_hacker_confirm(void) {
    buzzer_play_tone(NOTE_E5, 60);
    vTaskDelay(pdMS_TO_TICKS(30));
    buzzer_play_tone(NOTE_G5, 100);
}

// Efeito: Flipper access granted (melodia curta estilo Flipper)
void buzzer_flipper_granted(void) {
    buzzer_play_tone(NOTE_A4, 60);
    buzzer_play_tone(NOTE_C5, 60);
    buzzer_play_tone(NOTE_E5, 120);
}

// Efeito: Flipper denied (negação com tom quebrado)
void buzzer_flipper_denied(void) {
    buzzer_play_tone(NOTE_DS4, 80);
    buzzer_play_tone(NOTE_C4, 50);
    vTaskDelay(pdMS_TO_TICKS(80));
    buzzer_play_tone(NOTE_B3, 120);
}

// Música: Tema do Super Mario Bros (Overworld)
void buzzer_play_mario_theme(void) {
    const uint16_t melody[] = {
        NOTE_E7, NOTE_E7, 0, NOTE_E7,
        0, NOTE_C7, NOTE_E7, 0,
        NOTE_G7, 0, 0,  0,
        NOTE_G6, 0, 0, 0,
        NOTE_C7, 0, 0, NOTE_G6,
        0, 0, NOTE_E6, 0,
        0, NOTE_A6, 0, NOTE_B6,
        0, NOTE_AS6, NOTE_A6, 0,
        NOTE_G6, NOTE_E7, NOTE_G7,
        NOTE_A7, 0, NOTE_F7, NOTE_G7,
        0, NOTE_E7, 0, NOTE_C7,
        NOTE_D7, NOTE_B6, 0, 0
    };
    const uint8_t tempo[] = {
        12, 12, 12, 12,
        12, 12, 12, 12,
        12, 12, 12, 12,
        12, 12, 12, 12,
        12, 12, 12, 12,
        12, 12, 12, 12,
        12, 12, 12, 12,
        12, 12, 12, 12,
        9, 9, 9,
        12, 12, 12, 12,
        12, 12, 12, 12,
        12, 12, 12, 12,
        12, 12, 12, 12,
        9, 9, 9,
        12, 12, 12, 12,
        12, 12, 12, 12,
        12, 12, 12, 12
    };
    int notes = sizeof(melody)/sizeof(melody[0]);
    for (int i = 0; i < notes; i++) {
        uint16_t note = melody[i];
        uint32_t dur = 1000 / tempo[i];
        if (note == 0) {
            // Pausa (repouso)
            vTaskDelay(dur / portTICK_PERIOD_MS);
        } else {
            buzzer_play_tone(note, dur);
        }
        // Pausa extra entre notas (~30% do tempo da nota)
        vTaskDelay((dur * 3 / 10) / portTICK_PERIOD_MS);
    }
}

// Música: Zelda's Lullaby (Ocarina of Time)
void buzzer_play_zeldas_lullaby(void) {
    const uint16_t melody[] = {
        NOTE_B3, NOTE_D4, NOTE_A3, NOTE_G3, NOTE_A3, NOTE_B3, NOTE_D4, NOTE_A3,
        NOTE_B3, NOTE_D4, NOTE_A4, NOTE_G4, NOTE_D4, NOTE_C4, NOTE_B3, NOTE_A3,
        NOTE_B3, NOTE_D4, NOTE_A3, NOTE_G3, NOTE_A3, NOTE_B3, NOTE_D4, NOTE_A3,
        NOTE_B3, NOTE_D4, NOTE_A4, NOTE_G4, NOTE_D5
    };
    const uint16_t durations[] = {
        1000, 500, 1000, 250, 250, 1000, 500, 1500,
        1000, 500, 1000, 500, 1000, 250, 250, 1500,
        1000, 500, 1000, 250, 250, 1000, 500, 1500,
        1000, 500, 1000, 500, 1500
    };
    int notes = sizeof(melody)/sizeof(melody[0]);
    for (int i = 0; i < notes; i++) {
        uint16_t note = melody[i];
        uint32_t dur = durations[i];
        buzzer_play_tone(note, dur);
        // Pausa extra (~30% do tempo da nota)
        vTaskDelay((dur * 3 / 10) / portTICK_PERIOD_MS);
    }
}

void buzzer_play_megalovania(void) {
    const uint16_t melody[] = {
        NOTE_D5, NOTE_D5, NOTE_A4, NOTE_D5, NOTE_A4, NOTE_D5, NOTE_A4, 0,
        NOTE_F5, NOTE_F5, NOTE_C5, NOTE_F5, NOTE_C5, NOTE_F5, NOTE_C5, 0,
        NOTE_D5, NOTE_D5, NOTE_A4, NOTE_D5, NOTE_A4, NOTE_D5, NOTE_A4, 0,
        NOTE_G5, NOTE_FS5, NOTE_F5, NOTE_DS5, NOTE_D5, NOTE_DS5, NOTE_F5,
        NOTE_F5, NOTE_F5, NOTE_F5, NOTE_D5, NOTE_A4, NOTE_A4,
        NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, NOTE_D5, NOTE_D5, NOTE_A4, NOTE_A4,
        NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, NOTE_D5, NOTE_D5, 0
    };

    const uint16_t tempo[] = {
        8, 8, 8, 8, 8, 8, 4, 8,
        8, 8, 8, 8, 8, 8, 4, 8,
        8, 8, 8, 8, 8, 8, 4, 8,
        8, 8, 8, 8, 8, 8, 4,
        8, 8, 4, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 4
    };

    int notes = sizeof(melody)/sizeof(melody[0]);

    for (int i = 0; i < notes; i++) {
        uint16_t note = melody[i];
        uint32_t dur = 1000 / tempo[i];

        if (note == 0) {
            vTaskDelay(pdMS_TO_TICKS(dur));
        } else {
            buzzer_play_tone(note, dur);
        }

        // Pequena pausa entre as notas
        vTaskDelay(pdMS_TO_TICKS(dur * 0.2));
    }
}


