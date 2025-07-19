#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "st7789.h"
#include "icons.h"
#include "UART.h"
#include "pin_def.h"
#include "sub_menu.h" 

static const SubMenuItem GPIOMenuItems[] = {
    { "MONITOR UART", UART, uart_monitor_start },      // Abre o notepad e escreve uma msg

    // Adicione mais payloads aqui...
};
static const int GPIOMenuSize = sizeof(GPIOMenuItems) / sizeof(SubMenuItem);

// --- Ação Principal: Mostrar a lista de Payloads ---
// Esta função é chamada quando o usuário seleciona "Payloads" no menu principal do BadUSB.
void show_gpio_menu(void) {
    // Mostra um novo submenu com a lista de payloads que definimos acima.
    show_submenu(GPIOMenuItems, GPIOMenuSize, "Menu GPIOS");
}