#include "sub_menu.h"
#include "st7789.h"
#include "buzzer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <stdio.h>
#include <string.h>

// ==== PROTÓTIPOS DAS FUNÇÕES ====
void play_mario(void);
void play_zelda(void);
void play_pop(void);
void play_alarm(void);
void play_success(void);
void play_hacker_glitch(void);

void show_music_status(const char *title, void (*play_func)(void));

// ==== MENU DE MUSICAS ====
static const MenuItem musicMenu[] = {
    {"Mario Theme", NULL, play_mario},
    {"Zelda Lullaby", NULL, play_zelda},
    {"Pop Music", NULL, play_pop},
    {"Alarm", NULL, play_alarm},
    {"Success", NULL, play_success},
    {"Hacker Glitch", NULL, play_hacker_glitch},
};

void show_music_menu(void) {
    show_menu(musicMenu, sizeof(musicMenu) / sizeof(MenuItem), "Musicas");
    handle_menu_controls();
}

// ==== TASK GENÉRICA PARA TOCAR ====
void task_play_music(void *param) {
    void (**args)(void) = (void (**)(void))param;
    void (*func)(void) = args[0];
    func();
    vTaskDelete(NULL);
}

// ==== TELA DE STATUS COM NOME DA MÚSICA ====
void show_music_status(const char *title, void (*play_func)(void)) {
    // Desenha tela de status
    st7789_fill_screen(ST7789_COLOR_BLACK);
    st7789_draw_round_rect(10, 30, 220, 180, 10, ST7789_COLOR_PURPLE);
    st7789_set_text_size(2);
    st7789_draw_text(30, 50, "Tocando:", ST7789_COLOR_WHITE);
    st7789_set_text_size(2);
    st7789_draw_text(30, 80, title, ST7789_COLOR_LIGHT_PURPLE);

    st7789_set_text_size(1);
    st7789_draw_text(30, 200, "Pressione BACK para voltar", ST7789_COLOR_GRAY);

    // Cria uma task para tocar a música
    void (*param[1])(void);
    param[0] = play_func;
    xTaskCreate(task_play_music, "task_play_music", 2048, param, 5, NULL);

    // Espera até o usuário apertar BACK
    while (1) {
        if (!gpio_get_level(7)) { // BTN_BACK
            vTaskDelay(pdMS_TO_TICKS(200));
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // Volta para o menu de músicas
    show_music_menu();
}

// ==== IMPLEMENTAÇÃO DAS FUNÇÕES ====

void play_mario(void) {
    show_music_status("Mario Theme", buzzer_play_mario_theme);
}

void play_zelda(void) {
    show_music_status("Zelda Lullaby", buzzer_play_zeldas_lullaby);
}

void play_pop(void) {
    extern void play_pop_music(void);
    show_music_status("Pop Music", play_pop_music);
}

void play_alarm(void) {
    show_music_status("Alarm", buzzer_alarm);
}

void play_success(void) {
    show_music_status("Success", buzzer_success);
}

void play_hacker_glitch(void) {
    show_music_status("Hacker Glitch", buzzer_hacker_glitch);
}
