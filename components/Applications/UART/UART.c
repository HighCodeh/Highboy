#include "UART.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "st7789.h"
#include "pin_def.h"

// --- Definições e Variáveis Estáticas ---

static const char *TAG = "UART_MONITOR";

// Configurações da UART
#define UART_PORT_NUM       UART_NUM_1
#define UART_RX_PIN         44
#define UART_TX_PIN         43
#define UART_BUFFER_SIZE    1024

// Lógica do Baud Rate
static const int BAUD_RATES[] = {9600, 19200, 38400, 57600, 115200};
static int baud_rate_index = 4;

// Lógica de Buffer e Rolagem
#define VISIBLE_LINES 15
#define HISTORY_BUFFER_LINES 100
#define MAX_COLS  40

static char history_buffer[HISTORY_BUFFER_LINES][MAX_COLS];
static int head_index = 0;
static int total_lines_in_buffer = 0;
static int view_offset = 0;

// Controle do loop
static bool is_running = false;
static TaskHandle_t uart_reader_task_handle = NULL;


// --- Funções Internas ---

static void clear_history_buffer() {
    for (int i = 0; i < HISTORY_BUFFER_LINES; i++) {
        memset(history_buffer[i], 0, MAX_COLS);
    }
    head_index = 0;
    total_lines_in_buffer = 0;
    view_offset = 0;
}

static void history_buffer_add_char(char c) {
    if (c == '\n' || c == '\r') {
        if(c == '\r' && history_buffer[head_index][0] != 0) return;
        head_index = (head_index + 1) % HISTORY_BUFFER_LINES;
        memset(history_buffer[head_index], 0, MAX_COLS);
        if (total_lines_in_buffer < HISTORY_BUFFER_LINES) {
            total_lines_in_buffer++;
        }
    } else if (c >= ' ') {
        int len = strlen(history_buffer[head_index]);
        if (len < MAX_COLS - 1) {
            history_buffer[head_index][len] = c;
            history_buffer[head_index][len + 1] = '\0';
        } else {
            history_buffer_add_char('\n');
            history_buffer_add_char(c);
        }
    }
    if (total_lines_in_buffer > VISIBLE_LINES) {
        view_offset = total_lines_in_buffer - VISIBLE_LINES;
    } else {
        view_offset = 0;
    }
}

// --- NOVO: Função da Barra de Rolagem (baseada na sua referência) ---
static void draw_uart_scrollbar() {
    // Só desenha se o conteúdo for maior que a tela
    if (total_lines_in_buffer <= VISIBLE_LINES) return;

    // Constantes para a barra de rolagem
    const int SCROLLBAR_WIDTH = 6;
    const int SCROLLBAR_Y_START = 30;
    const int SCROLLBAR_HEIGHT = 208; // Altura da área de texto

    // Desenha o fundo da barra (trilha)
    st7789_draw_vline_fb(ST7789_WIDTH - SCROLLBAR_WIDTH / 2, SCROLLBAR_Y_START, SCROLLBAR_HEIGHT, ST7789_COLOR_PURPLE);

    // Calcula a altura do indicador (thumb)
    float thumb_height = ((float)VISIBLE_LINES / total_lines_in_buffer) * SCROLLBAR_HEIGHT;
    if (thumb_height < 10) thumb_height = 10; // Altura mínima para ser visível

    // Calcula a posição Y do indicador
    float max_scroll_range = total_lines_in_buffer - VISIBLE_LINES;
    float scroll_percentage = (float)view_offset / max_scroll_range;
    float thumb_y = SCROLLBAR_Y_START + (scroll_percentage * (SCROLLBAR_HEIGHT - thumb_height));
    
    // Desenha o indicador
    st7789_fill_rect_fb(ST7789_WIDTH - SCROLLBAR_WIDTH, (int)thumb_y, SCROLLBAR_WIDTH, (int)thumb_height, ST7789_COLOR_PURPLE);
}


static void redraw_screen() {
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    st7789_set_text_size(2);
    
    char status_str[32];
    sprintf(status_str, "Baud: %d", BAUD_RATES[baud_rate_index]);
    st7789_draw_text_fb(5, 5, status_str, ST7789_COLOR_PURPLE, ST7789_COLOR_BLACK);
    st7789_draw_hline_fb(0, 25, ST7789_WIDTH, ST7789_COLOR_WHITE);
    st7789_set_text_size(1);

    for (int i = 0; i < VISIBLE_LINES; i++) {
        if (view_offset + i < total_lines_in_buffer) {
            int line_to_draw_index = (head_index - (total_lines_in_buffer - 1) + view_offset + i + HISTORY_BUFFER_LINES) % HISTORY_BUFFER_LINES;
            st7789_draw_text_fb(5, 35 + (i * 14), history_buffer[line_to_draw_index], ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
        }
    }
    
    // Chama a nova função para desenhar a barra de rolagem
    draw_uart_scrollbar();

    st7789_flush();
}

static void uart_reader_task(void *pvParameters) {
    uint8_t *data = (uint8_t *) malloc(UART_BUFFER_SIZE);
    bool needs_redraw = false;

    while (is_running) {
        int len = uart_read_bytes(UART_PORT_NUM, data, UART_BUFFER_SIZE, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                history_buffer_add_char((char)data[i]);
            }
            needs_redraw = true;
        }

        if (needs_redraw) {
            redraw_screen();
            needs_redraw = false;
        }
    }
    free(data);
    uart_reader_task_handle = NULL;
    vTaskDelete(NULL);
}

// --- Funções Públicas ---
// (init, deinit e start continuam iguais, pois a lógica de botões já estava correta)
void uart_monitor_init(void) {
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATES[baud_rate_index],
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(UART_PORT_NUM, UART_BUFFER_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_LOGI(TAG, "Hardware UART inicializado.");
}

void uart_monitor_deinit(void) {
    uart_driver_delete(UART_PORT_NUM);
    ESP_LOGI(TAG, "Hardware UART desligado.");
}

// Em seu arquivo UART.c

void uart_monitor_start(void) {
    // --- NOVO: Bloco de configuração dos botões com PULL-UP ---
    // Isso resolve o problema de "toques fantasma"
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BTN_UP) | (1ULL << BTN_DOWN) |
                        (1ULL << BTN_LEFT) | (1ULL << BTN_RIGHT) |
                        (1ULL << BTN_OK) | (1ULL << BTN_BACK),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, // Habilita o pull-up interno
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    // --- FIM DO NOVO BLOCO ---

    uart_monitor_init();
    is_running = true;
    
    clear_history_buffer();
    redraw_screen();

    xTaskCreate(uart_reader_task, "uart_reader_task", 4096, NULL, 10, &uart_reader_task_handle);

    while (is_running) {
        if (gpio_get_level(BTN_UP) == 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            if (view_offset > 0) {
                view_offset--;
                redraw_screen();
            }
        }
        if (gpio_get_level(BTN_DOWN) == 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            if (view_offset < (total_lines_in_buffer - VISIBLE_LINES)) {
                view_offset++;
                redraw_screen();
            }
        }
        if (gpio_get_level(BTN_LEFT) == 0) {
            vTaskDelay(pdMS_TO_TICKS(150));
            if (baud_rate_index == 0) baud_rate_index = (sizeof(BAUD_RATES)/sizeof(BAUD_RATES[0])) - 1;
            else baud_rate_index--;
            uart_set_baudrate(UART_PORT_NUM, BAUD_RATES[baud_rate_index]);
            redraw_screen();
        }
        if (gpio_get_level(BTN_RIGHT) == 0) {
            vTaskDelay(pdMS_TO_TICKS(150));
            baud_rate_index = (baud_rate_index + 1) % (sizeof(BAUD_RATES) / sizeof(BAUD_RATES[0]));
            uart_set_baudrate(UART_PORT_NUM, BAUD_RATES[baud_rate_index]);
            redraw_screen();
        }
        if (gpio_get_level(BTN_OK) == 0) {
            vTaskDelay(pdMS_TO_TICKS(150));
            const char* cmd = "help\r\n";
            uart_write_bytes(UART_PORT_NUM, cmd, strlen(cmd));
        }
        if (gpio_get_level(BTN_BACK) == 0) {
            vTaskDelay(pdMS_TO_TICKS(150));
            is_running = false;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (uart_reader_task_handle != NULL) {
        vTaskDelete(uart_reader_task_handle);
        uart_reader_task_handle = NULL;
    }
    uart_monitor_deinit();
    ESP_LOGI(TAG, "Saindo do Monitor UART.");
}