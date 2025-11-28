#include "keyboard_ui.h"
#include "st7789.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pin_def.h"
#include "driver/gpio.h"

// ---- Configurações de Layout ----
#define KEYBOARD_X_OFFSET 4
#define KEY_WIDTH 23
#define KEY_HEIGHT 23
#define KEY_H_SPACING 1
#define KEY_V_SPACING 1
#define KEYBOARD_Y_OFFSET (240 - (5 * KEY_HEIGHT + 4 * KEY_V_SPACING)) 

// ---- Tema de Cores ----
#define COLOR_BACKGROUND        ST7789_COLOR_BLACK
#define COLOR_KEY_FILL          ST7789_COLOR_DARK_PURPLE
#define COLOR_KEY_BORDER        ST7789_COLOR_PURPLE
#define COLOR_KEY_TEXT          ST7789_COLOR_WHITE
#define COLOR_SELECTED_FILL     ST7789_COLOR_LIGHT_PURPLE
#define COLOR_SELECTED_TEXT     ST7789_COLOR_WHITE
#define COLOR_INPUT_BG          ST7789_COLOR_DARK_PURPLE
#define COLOR_INPUT_TEXT        ST7789_COLOR_WHITE
#define COLOR_PROMPT_TEXT       ST7789_COLOR_YELLOW

// Enumeração para os layouts do teclado
typedef enum {
    LAYOUT_LOWERCASE,
    LAYOUT_UPPERCASE,
    LAYOUT_SYMBOLS
} keyboard_layout_t;

// Variáveis estáticas para controlar o estado do teclado
static char* text_buffer;
static int buffer_max_size;
static int cursor_x = 0;
static int cursor_y = 0;
static keyboard_layout_t current_layout = LAYOUT_LOWERCASE;

// Definições das dimensões do layout
#define NUM_ROWS 5
#define NUM_COLS 10

// Layouts do Teclado
const char* layout_lowercase[NUM_ROWS][NUM_COLS] = {
    {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
    {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p"},
    {"a", "s", "d", "f", "g", "h", "j", "k", "l", ";"},
    {"^", "z", "x", "c", "v", "b", "n", "m", ",", "."},
    {"Sym", " ", "", "", "", "", "Del", "", "Sav", ""}
};

const char* layout_uppercase[NUM_ROWS][NUM_COLS] = {
    {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
    {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"},
    {"A", "S", "D", "F", "G", "H", "J", "K", "L", ":"},
    {"^", "Z", "X", "C", "V", "B", "N", "M", "!", "?"},
    {"Sym", " ", "", "", "", "", "Del", "", "Sav", ""}
};

const char* layout_symbols[NUM_ROWS][NUM_COLS] = {
    {"+", "-", "*", "/", "=", "%", "&", "|", "<", ">"},
    {"@", "#", "$", "_", "(", ")", "[", "]", "{", "}"},
    {"`", "~", "\\", "^", "\"", "'", ".", ",", "?", "!"},
    {"", "", "", "", "", "", "", "", "", ""},
    {"abc", " ", "", "", "", "", "Del", "", "Sav", ""}
};

// Protótipos de funções estáticas
static void draw_keyboard(const char* prompt);
static int handle_key_press();

// Função de desenho (sem mudanças na lógica, apenas organização)
static void draw_keyboard(const char* prompt) {
    st7789_fill_screen_fb(COLOR_BACKGROUND);
    
    // 1. Desenha prompt e caixa de input
    st7789_set_text_size(1);
    if (prompt) {
        st7789_draw_text_fb(5, 60, prompt, COLOR_PROMPT_TEXT, COLOR_BACKGROUND);
    }
    st7789_fill_rect_fb(5, 80, ST7789_WIDTH - 10, 32, COLOR_INPUT_BG);
    st7789_set_text_size(2);
    st7789_draw_text_fb(8, 88, text_buffer, COLOR_INPUT_TEXT, COLOR_INPUT_BG);

    // 2. Volta para a fonte pequena para as teclas
    st7789_set_text_size(1);
    const char* (*current_keys)[NUM_COLS] = (current_layout == LAYOUT_LOWERCASE) ? layout_lowercase : (current_layout == LAYOUT_UPPERCASE) ? layout_uppercase : layout_symbols;

    // 3. Desenha todas as teclas
    for (int row = 0; row < NUM_ROWS; row++) {
        for (int col = 0; col < NUM_COLS; col++) {
            const char* key_text = current_keys[row][col];
            if (strlen(key_text) == 0) continue;

            int key_x = KEYBOARD_X_OFFSET + col * (KEY_WIDTH + KEY_H_SPACING);
            int key_y = KEYBOARD_Y_OFFSET + row * (KEY_HEIGHT + KEY_V_SPACING);
            int key_w = KEY_WIDTH;

            // Lógica para teclas largas da última fileira
            if (row == 4) {
                 if (strcmp(key_text, " ") == 0) {
                    key_x = KEYBOARD_X_OFFSET + 1 * (KEY_WIDTH + KEY_H_SPACING);
                    key_w = KEY_WIDTH * 5 + KEY_H_SPACING * 4; // Respeitando a sua alteração para 5 teclas
                 } else if (strcmp(key_text, "Del") == 0) { // CORRIGIDO: "Bsp" para "Del"
                    key_x = KEYBOARD_X_OFFSET + 6 * (KEY_WIDTH + KEY_H_SPACING);
                    key_w = KEY_WIDTH * 2 + KEY_H_SPACING;
                 } else if (strcmp(key_text, "Sav") == 0) {
                    key_x = KEYBOARD_X_OFFSET + 8 * (KEY_WIDTH + KEY_H_SPACING);
                    key_w = KEY_WIDTH * 2 + KEY_H_SPACING;
                 }
            }

            bool is_selected = (row == cursor_y && col == cursor_x);
            uint16_t fill_color = is_selected ? COLOR_SELECTED_FILL : COLOR_KEY_FILL;
            uint16_t text_color = is_selected ? COLOR_SELECTED_TEXT : COLOR_KEY_TEXT;
            
            st7789_fill_round_rect_fb(key_x, key_y, key_w, KEY_HEIGHT, 3, fill_color);
            st7789_draw_round_rect_fb(key_x, key_y, key_w, KEY_HEIGHT, 3, COLOR_KEY_BORDER);

            int text_len = strlen(key_text);
            int text_x = key_x + (key_w - (text_len * 6)) / 2;
            int text_y = key_y + (KEY_HEIGHT - 8) / 2;
            st7789_draw_text_fb(text_x, text_y, key_text, text_color, fill_color);
        }
    }
}

// Lógica de ação da tecla pressionada
static int handle_key_press() {
    const char* key_pressed = layout_lowercase[cursor_y][cursor_x];
    switch (current_layout) {
        case LAYOUT_UPPERCASE: key_pressed = layout_uppercase[cursor_y][cursor_x]; break;
        case LAYOUT_SYMBOLS:   key_pressed = layout_symbols[cursor_y][cursor_x];   break;
        default: break;
    }

    if (strcmp(key_pressed, "^") == 0) current_layout = (current_layout == LAYOUT_LOWERCASE) ? LAYOUT_UPPERCASE : LAYOUT_LOWERCASE;
    else if (strcmp(key_pressed, "Sym") == 0) current_layout = LAYOUT_SYMBOLS;
    else if (strcmp(key_pressed, "abc") == 0) current_layout = LAYOUT_LOWERCASE;
    else if (strcmp(key_pressed, "Del") == 0) { // CORRIGIDO: "Bsp" para "Del"
        int len = strlen(text_buffer);
        if (len > 0) text_buffer[len - 1] = '\0';
    } else if (strcmp(key_pressed, "Sav") == 0) {
        return 1;
    } else {
        int len = strlen(text_buffer);
        if (len < buffer_max_size - 1) strcat(text_buffer, key_pressed);
    }
    return 0;
}

// ** REATORADO: Função de leitura de botões mais limpa e eficiente **
int get_single_button_press() {
    const int buttons[] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_OK, BTN_BACK};
    const int num_buttons = sizeof(buttons) / sizeof(buttons[0]);
    int pressed_button = -1;

    // 1. Espera soltar qualquer botão que já esteja pressionado
    bool any_pressed;
    do {
        any_pressed = false;
        for (int i = 0; i < num_buttons; i++) {
            if (gpio_get_level(buttons[i]) == 0) {
                any_pressed = true;
                break;
            }
        }
        if (any_pressed) vTaskDelay(pdMS_TO_TICKS(20));
    } while (any_pressed);

    // 2. Espera por um novo pressionamento
    while (true) {
        for (int i = 0; i < num_buttons; i++) {
            if (gpio_get_level(buttons[i]) == 0) {
                // Botão pressionado, aguarda debounce
                vTaskDelay(pdMS_TO_TICKS(50));
                // Confirma se ainda está pressionado
                if (gpio_get_level(buttons[i]) == 0) {
                    // Espera soltar o botão
                    while (gpio_get_level(buttons[i]) == 0) {
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                    return buttons[i]; // Retorna o botão que foi pressionado
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// Função principal do módulo de teclado
bool show_keyboard_ui(char* buffer, int buffer_size, const char* prompt) {
    text_buffer = buffer;
    buffer_max_size = buffer_size;
    cursor_x = 0;
    cursor_y = 0;
    current_layout = LAYOUT_LOWERCASE;
    
    while (1) {
        draw_keyboard(prompt);
        st7789_flush();
        int button = get_single_button_press();
        
        if (button == BTN_OK) {
            if (handle_key_press() == 1) { // Ação foi "Salvar"
                st7789_set_text_size(2); 
                return true;
            }
        } else if (button == BTN_BACK) {
            if (strlen(text_buffer) > 0) {
                text_buffer[strlen(text_buffer) - 1] = '\0';
            } else { // Sai se o buffer estiver vazio
                st7789_set_text_size(2); 
                return false;
            }
        } else {
            // ** REATORADO: Lógica de navegação mais robusta **
            int dx = 0, dy = 0;
            if (button == BTN_RIGHT) dx = 1;
            if (button == BTN_LEFT) dx = -1;
            if (button == BTN_DOWN) dy = 1;
            if (button == BTN_UP) dy = -1;

            const char* (*keys)[NUM_COLS] = (current_layout == LAYOUT_LOWERCASE) ? layout_lowercase : (current_layout == LAYOUT_UPPERCASE) ? layout_uppercase : layout_symbols;
            
            // Procura a próxima tecla válida na direção do movimento
            do {
                cursor_x = (cursor_x + dx + NUM_COLS) % NUM_COLS;
                cursor_y = (cursor_y + dy + NUM_ROWS) % NUM_ROWS;
            } while (strlen(keys[cursor_y][cursor_x]) == 0);
        }
    }
}