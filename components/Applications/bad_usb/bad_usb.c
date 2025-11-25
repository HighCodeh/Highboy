// Copyright (c) 2025 HIGH CODE LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "tusb_desc.h"
#include "bad_usb.h"

// TAG para logs
static const char *TAG = "BAD_USB_MODULE";
#define REPORT_ID_KEYBOARD 1


// --- 2. FUNÇÕES AUXILIARES ESTÁTICAS ---
static void send_key(uint8_t keycode, uint8_t modifier) {
    uint8_t keycode_array[6] = {0};
    keycode_array[0] = keycode;
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycode_array);
    vTaskDelay(pdMS_TO_TICKS(10));
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
    vTaskDelay(pdMS_TO_TICKS(10));
}

static void type_string(const char* str) {
    for (size_t i = 0; str[i] != '\0'; ++i) {
        char c = str[i];
        uint8_t keycode = 0;
        uint8_t modifier = 0;
        if (c >= 'a' && c <= 'z') { keycode = HID_KEY_A + (c - 'a'); }
        else if (c >= 'A' && c <= 'Z') { keycode = HID_KEY_A + (c - 'A'); modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
        else if (c >= '1' && c <= '9') { keycode = HID_KEY_1 + (c - '1'); }
        else if (c == '0') { keycode = HID_KEY_0; }
        else {
            switch (c) {
                case ' ': keycode = HID_KEY_SPACE; break;
                case '!': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_1; break;
                case '@': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_2; break;
                case '#': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_3; break;
                case '$': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_4; break;
                case '%': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_5; break;
                case '^': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_6; break;
                case '&': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_7; break;
                case '*': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_8; break;
                case '(': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_9; break;
                case ')': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_0; break;
                case '-': keycode = HID_KEY_MINUS; break;
                case '_': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_MINUS; break;
                case '=': keycode = HID_KEY_EQUAL; break;
                case '+': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_EQUAL; break;
                case '.': keycode = HID_KEY_PERIOD; break;
                case ',': keycode = HID_KEY_COMMA; break;
                case '/': keycode = HID_KEY_SLASH; break;
                case '?': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_SLASH; break;
                case ':': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_SEMICOLON; break;
                case ';': keycode = HID_KEY_SEMICOLON; break;
            }
        }
        if (keycode != 0) send_key(keycode, modifier);
    }
}

void type_string_abnt2(const char* str) {
    for (size_t i = 0; str[i] != '\0'; ++i) {
        uint8_t c1 = (uint8_t)str[i];
        uint8_t c2 = (uint8_t)str[i+1];

        // --- LÓGICA PARA CARACTERES MULTI-BYTE (UTF-8) ---
        // Verifica se o byte atual é o início de uma sequência de 2 bytes
        if (c1 == '\'') { // Aspas Simples
            // Pressiona a tecla morta '´' e depois espaço
            send_key(HID_KEY_BRACKET_LEFT, 0);
            send_key(HID_KEY_SPACE, 0);
            continue; // Pula para o próximo caractere do loop
        }
        if (c1 == '"') { // Aspas Duplas
            // Pressiona SHIFT + tecla '´'
            send_key(HID_KEY_BRACKET_LEFT, KEYBOARD_MODIFIER_LEFTSHIFT);
            continue; // Pula para o próximo caractere do loop
        }
        if ((c1 & 0xE0) == 0xC0 && c2 != '\0') {
            bool char_processed = true;

            // Mapeamento para 'ç' e 'Ç'
            if (c1 == 0xC3 && c2 == 0xA7) { // ç
                send_key(HID_KEY_SEMICOLON, 0);
            } else if (c1 == 0xC3 && c2 == 0x87) { // Ç
                send_key(HID_KEY_SEMICOLON, KEYBOARD_MODIFIER_LEFTSHIFT);
            
            // Mapeamento para acentos agudos: ´ + vogal
            } else if (c1 == 0xC3 && c2 == 0xA1) { // á
                send_key(HID_KEY_BRACKET_LEFT, 0); send_key(HID_KEY_A, 0);
            } else if (c1 == 0xC3 && c2 == 0xA9) { // é
                send_key(HID_KEY_BRACKET_LEFT, 0); send_key(HID_KEY_E, 0);
            } else if (c1 == 0xC3 && c2 == 0xAD) { // í
                send_key(HID_KEY_BRACKET_LEFT, 0); send_key(HID_KEY_I, 0);
            } else if (c1 == 0xC3 && c2 == 0xB3) { // ó
                send_key(HID_KEY_BRACKET_LEFT, 0); send_key(HID_KEY_O, 0);
            } else if (c1 == 0xC3 && c2 == 0xBA) { // ú
                send_key(HID_KEY_BRACKET_LEFT, 0); send_key(HID_KEY_U, 0);

            // Mapeamento para acentos circunflexos: ^ + vogal
            } else if (c1 == 0xC3 && c2 == 0xA2) { // â
                send_key(HID_KEY_APOSTROPHE, KEYBOARD_MODIFIER_LEFTSHIFT); send_key(HID_KEY_A, 0);
            } else if (c1 == 0xC3 && c2 == 0xAA) { // ê
                send_key(HID_KEY_APOSTROPHE, KEYBOARD_MODIFIER_LEFTSHIFT); send_key(HID_KEY_E, 0);
            } else if (c1 == 0xC3 && c2 == 0xB4) { // ô
                send_key(HID_KEY_APOSTROPHE, KEYBOARD_MODIFIER_LEFTSHIFT); send_key(HID_KEY_O, 0);
            
            // Mapeamento para til: ~ + vogal
            } else if (c1 == 0xC3 && c2 == 0xA3) { // ã
                send_key(HID_KEY_APOSTROPHE, 0); send_key(HID_KEY_A, 0);
            } else if (c1 == 0xC3 && c2 == 0xB5) { // õ
                send_key(HID_KEY_APOSTROPHE, 0); send_key(HID_KEY_O, 0);
                
            // Mapeamento para crase: ` + a
            } else if (c1 == 0xC3 && c2 == 0xA0) { // à
                send_key(HID_KEY_BRACKET_LEFT, KEYBOARD_MODIFIER_LEFTSHIFT); send_key(HID_KEY_A, 0);
            
            } else {
                char_processed = false; // Não é um caractere especial que conhecemos
            }

            if (char_processed) {
                i++; // Pula o segundo byte do caractere UTF-8, pois já foi processado
                continue; // Vai para a próxima iteração do loop
            }
        }

        // --- LÓGICA PARA CARACTERES DE 1 BYTE (ASCII) ---
        uint8_t keycode = 0;
        uint8_t modifier = 0;
        
        if (c1 >= 'a' && c1 <= 'z') { keycode = HID_KEY_A + (c1 - 'a'); }
        else if (c1 >= 'A' && c1 <= 'Z') { modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_A + (c1 - 'A'); }
        else if (c1 >= '1' && c1 <= '9') { keycode = HID_KEY_1 + (c1 - '1'); }
        else if (c1 == '0') { keycode = HID_KEY_0; }
        else {
             switch (c1) {
                case '!': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_1; break;
                case '@': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_2; break;
                case '#': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_3; break;
                case '$': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_4; break;
                case '%': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_5; break;
                case '&': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_7; break;
                case '*': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_8; break;
                case '(': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_9; break;
                case ')': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_0; break;
                case ' ': keycode = HID_KEY_SPACE; break;
                case '\n': keycode = HID_KEY_ENTER; break;
                case '\t': keycode = HID_KEY_TAB; break;
                case '-': keycode = HID_KEY_MINUS; break;
                case '=': keycode = HID_KEY_EQUAL; break;
                case '_': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_MINUS; break;
                case '+': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_EQUAL; break;
                case '.': keycode = HID_KEY_PERIOD; break;
                case ',': keycode = HID_KEY_COMMA; break;
                case ';': keycode = HID_KEY_SLASH; break;
                case ':': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_SLASH; break;
                case '/': modifier = KEYBOARD_MODIFIER_RIGHTALT; keycode = HID_KEY_Q; break;
                case '?': modifier = KEYBOARD_MODIFIER_RIGHTALT; keycode = HID_KEY_W; break;
                case '[': modifier = KEYBOARD_MODIFIER_RIGHTALT; keycode = HID_KEY_BRACKET_LEFT; break;
                case '{': modifier = KEYBOARD_MODIFIER_RIGHTALT; keycode = HID_KEY_BRACKET_LEFT; modifier |= KEYBOARD_MODIFIER_LEFTSHIFT; break;
                case ']': modifier = KEYBOARD_MODIFIER_RIGHTALT; keycode = HID_KEY_BRACKET_RIGHT; break;
                case '}': modifier = KEYBOARD_MODIFIER_RIGHTALT; keycode = HID_KEY_BRACKET_RIGHT; modifier |= KEYBOARD_MODIFIER_LEFTSHIFT; break;
                case '\\': keycode = HID_KEY_BACKSLASH; break;
                case '|': modifier = KEYBOARD_MODIFIER_LEFTSHIFT; keycode = HID_KEY_BACKSLASH; break;
            }
        }
        if (keycode != 0) {
            send_key(keycode, modifier);
        }
    }
}


static void wait_for_connection(void) {
    ESP_LOGI(TAG, "Aguardando conexao USB para executar o payload...");
    while (!tud_mounted()) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_LOGI(TAG, "Dispositivo conectado. Executando em 2 segundos...");
    vTaskDelay(pdMS_TO_TICKS(2000));
}

// --- 3. FUNÇÕES PÚBLICAS ---
void bad_usb_init(void) {
  busb_init();
}

void bad_usb_deinit(void) {
    ESP_LOGI(TAG, "Finalizando o modo BadUSB e desinstalando o driver...");
    ESP_ERROR_CHECK(tinyusb_driver_uninstall());
    ESP_LOGI(TAG, "Driver TinyUSB desinstalado com sucesso.");
}

void run_payload_helloworld(void) {
    wait_for_connection();
    ESP_LOGI(TAG, "Executando Payload: Hello World");
    send_key(HID_KEY_R, KEYBOARD_MODIFIER_LEFTGUI);
    vTaskDelay(pdMS_TO_TICKS(500));
    type_string_abnt2("notepad");
    send_key(HID_KEY_ENTER, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    type_string_abnt2("Ola! Fui programado pela HighCode. :)");
}

void run_payload_powershell(void) {
    wait_for_connection();
    ESP_LOGI(TAG, "Executando Payload: Abrir PowerShell");
    send_key(HID_KEY_R, KEYBOARD_MODIFIER_LEFTGUI);
    vTaskDelay(pdMS_TO_TICKS(500));
    type_string_abnt2("powershell");
    send_key(HID_KEY_ENTER, 0);
}

void run_payload_rickroll(void) {
    wait_for_connection();
    ESP_LOGI(TAG, "Executando Payload: Rick Roll");
    send_key(HID_KEY_R, KEYBOARD_MODIFIER_LEFTGUI);
    vTaskDelay(pdMS_TO_TICKS(500));
    type_string_abnt2("https://www.youtube.com/watch?v=dQw4w9WgXcQ");
    send_key(HID_KEY_ENTER, 0);
}

void run_payload_linux_rickroll(void) {
    wait_for_connection();
    ESP_LOGI(TAG, "Executando Payload: Linux KDE Rick Roll");

    // 1. Abre o KRunner
    ESP_LOGI(TAG, "Abrindo o KRunner com Alt+Espaco...");
    send_key(HID_KEY_SPACE, KEYBOARD_MODIFIER_LEFTALT);

    // Delay um pouco maior para garantir que o KRunner esteja pronto.
    vTaskDelay(pdMS_TO_TICKS(1200));

    // 2. Digita o nome do terminal e o abre
    ESP_LOGI(TAG, "Abrindo terminal...");
    type_string_abnt2("Ghostty");
    send_key(HID_KEY_ENTER, 0);

    // DELAY CRÍTICO: Aumentado para 2 segundos para dar tempo ao terminal
    // de carregar completamente e receber o foco do teclado.
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 3. Digita e executa o comando para abrir o link
    type_string_abnt2("firefox 'https://github.com/HighCodeh/Highboy'");
    send_key(HID_KEY_ENTER, 0);
    
    ESP_LOGI(TAG, "Payload enviado.");
}
