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
#include "driver/gpio.h"
#include "esp_log.h"
#include "st7789.h"
#include "icons.h"
#include "pin_def.h"
#include "sub_menu.h" // Framework do menu
#include "bad_usb.h"  // Nosso módulo BadUSB

// --- Protótipos das Ações dos Payloads ---
// Cada payload terá uma função de ação que prepara e chama a execução.
static void payload_action_hello(void);
static void payload_action_powershell(void);
static void payload_action_rickroll(void);
static void payload_action_linux_rickroll(void);
// Adicione mais protótipos de payloads aqui...

// --- Lista de Itens do Menu de Payloads ---
// Cada item define o texto que aparece na tela e a função que será chamada.
static const SubMenuItem payloadMenuItems[] = {
    { "Hello World",   bad, payload_action_hello },      // Abre o notepad e escreve uma msg
    { "Abrir PowerShell", bad, payload_action_powershell }, // Abre o terminal PowerShell no Windows
    { "Rick Roll",     bad, payload_action_rickroll },   // Abre o navegador no vídeo clássico
    { "Linux Rick Roll",     bad,  payload_action_linux_rickroll },
    // Adicione mais payloads aqui...
};
static const int payloadMenuSize = sizeof(payloadMenuItems) / sizeof(SubMenuItem);

// --- Ação Principal: Mostrar a lista de Payloads ---
// Esta função é chamada quando o usuário seleciona "Payloads" no menu principal do BadUSB.
static void bad_usb_action_payloads(void) {
    // Mostra um novo submenu com a lista de payloads que definimos acima.
    show_submenu(payloadMenuItems, payloadMenuSize, "Selecionar Payload");
}

// --- Menu Principal do Módulo BadUSB ---
// Este é o primeiro menu que o usuário vê ao entrar na função BadUSB.
static const SubMenuItem badUsbMenuItems[] = {
    { "Payloads", evil, bad_usb_action_payloads },
};
static const int badUsbMenuSize = sizeof(badUsbMenuItems) / sizeof(SubMenuItem);

// --- Função de Entrada do Módulo ---
// Chame esta função a partir do seu menu principal para iniciar o modo BadUSB.
void show_bad_usb_menu(void) {
    // Inicializa o driver USB uma vez ao entrar no menu.
    // Isso prepara o ESP32 para agir como um dispositivo USB.
    bad_usb_init(); 
    
    // Mostra o menu principal do BadUSB ("Payloads")
    show_submenu(badUsbMenuItems, badUsbMenuSize, "Menu BadUSB");
}


//==============================================================================
// FUNÇÕES AUXILIARES E IMPLEMENTAÇÕES DAS AÇÕES
//==============================================================================

/**
 * @brief Mostra uma tela de instrução e espera o usuário conectar o cabo USB.
 * * @param payload_name O nome do payload que será executado.
 */
static void show_execution_screen(const char* payload_name) {
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    st7789_draw_text_fb(10, 40, "Executando Payload:", ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK);
    st7789_draw_text_fb(10, 70, payload_name, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    
    st7789_draw_text_fb(10, 150, "Conecte o cabo USB", ST7789_COLOR_CYAN, ST7789_COLOR_BLACK);
    st7789_draw_text_fb(10, 180, "no computador alvo!", ST7789_COLOR_CYAN, ST7789_COLOR_BLACK);
    st7789_flush(); // Envia o buffer para a tela
}

// --- Implementação das Ações de cada Payload ---

static void payload_action_hello(void) {
    show_execution_screen("Hello World");
    
    // Chama a função específica do módulo bad_usb para executar este payload.
    // A função run_payload_... cuidará de esperar a conexão e digitar.
    run_payload_helloworld();
    
    // Feedback de conclusão
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    st7789_draw_text_fb(40, 110, "Payload Concluido!", ST7789_COLOR_GREEN, ST7789_COLOR_BLACK);
    st7789_flush();
    vTaskDelay(pdMS_TO_TICKS(2000));
}

static void payload_action_powershell(void) {
    show_execution_screen("Abrir PowerShell");
    run_payload_powershell();
    
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    st7789_draw_text_fb(40, 110, "Payload Concluido!", ST7789_COLOR_GREEN, ST7789_COLOR_BLACK);
    st7789_flush();
    vTaskDelay(pdMS_TO_TICKS(2000));
}

static void payload_action_rickroll(void) {
    show_execution_screen("Rick Roll");
    run_payload_rickroll();
    
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    st7789_draw_text_fb(40, 110, "Payload Concluido!", ST7789_COLOR_GREEN, ST7789_COLOR_BLACK);
    st7789_flush();
    vTaskDelay(pdMS_TO_TICKS(2000));
}

static void payload_action_linux_rickroll(void) {
    show_execution_screen("Linux Rick Roll");
    run_payload_linux_rickroll(); // Chama a função que acabamos de criar
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    st7789_draw_text_fb(40, 110, "Payload Concluido!", ST7789_COLOR_GREEN, ST7789_COLOR_BLACK);
    st7789_flush();
    vTaskDelay(pdMS_TO_TICKS(2000));
}
