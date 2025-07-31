#include "sd_browser.h"
#include "storage_api.h"
#include "sub_menu.h"
#include "st7789.h"
#include "icons.h"
#include "pin_def.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "driver/gpio.h"

// Função para mostrar o conteúdo do ficheiro (do seu código antigo)
static void show_file_content_screen(const char* filename, const char* content) {
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
    menu_draw_header(filename); // Usa o cabeçalho do novo UI
    st7789_set_text_size(1);
    st7789_draw_text_fb(5, 35, content, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    st7789_flush();
    vTaskDelay(pdMS_TO_TICKS(500));
    while (gpio_get_level(BTN_BACK)) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Nova função que combina a lógica antiga com o UI novo
void sd_browser_start(void) {
    // Aloca memória para a lista
    file_list_t* file_list = malloc(sizeof(file_list_t));
    if (file_list == NULL) return;

    // Tenta listar os ficheiros da raiz "/"
    if (storage_list_files("/", file_list) != ESP_OK) {
        menu_draw_header("Erro no SD Card");
        st7789_draw_text_fb(20, 100, "Erro ao ler o cartao SD!", ST7789_COLOR_RED, ST7789_COLOR_BLACK);
        st7789_flush();
        vTaskDelay(pdMS_TO_TICKS(2000));
        free(file_list);
        return;
    }

    // Prepara os itens para o menu do sub_menu.c
    SubMenuItem* sub_menu_items = malloc(file_list->count * sizeof(SubMenuItem));
    if (!sub_menu_items) {
        free(file_list);
        return;
    }

    for (int i = 0; i < file_list->count; i++) {
        sub_menu_items[i].label = file_list->names[i];
        // Define o ícone com base se é diretório ou não
        sub_menu_items[i].icon = file_list->is_dir[i] ? icon_folder : icon_file;
        sub_menu_items[i].action = NULL;
    }

    // Mostra o menu usando a nova UI e aguarda a seleção do usuário
    int selected_index = show_picker_menu(sub_menu_items, file_list->count, "MicroSD");

    // Se o usuário selecionou um item (pressionou OK)
    if (selected_index >= 0) {
        // Verifica se o item selecionado NÃO é um diretório
        if (!file_list->is_dir[selected_index]) {
            char* file_content = malloc(2048);
            if (file_content) {
                size_t bytes_read;
                char file_path[256];
                // Cria o caminho do ficheiro, sempre a partir da raiz "/"
                snprintf(file_path, sizeof(file_path), "/%s", file_list->names[selected_index]);

                if (storage_read_file(file_path, file_content, 2048, &bytes_read) == ESP_OK) {
                    show_file_content_screen(file_list->names[selected_index], file_content);
                } else {
                     // Em caso de erro na leitura
                    st7789_fill_screen_fb(ST7789_COLOR_BLACK);
                    menu_draw_header("Erro");
                    st7789_draw_text_fb(10, 100, "Erro ao ler ficheiro!", ST7789_COLOR_RED, ST7789_COLOR_BLACK);
                    st7789_flush();
                    vTaskDelay(pdMS_TO_TICKS(1500));
                }
                free(file_content);
            }
        }
    }
    
    // Liberta a memória alocada
    free(sub_menu_items);
    free(file_list);
}