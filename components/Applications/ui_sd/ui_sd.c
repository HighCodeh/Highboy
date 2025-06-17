
#include "ui_sd.h"
#include "hb_sd.h"
#include "st7789.h"
#include "driver/gpio.h"
#include "sdmmc_cmd.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define COLOR_BG        ST7789_COLOR_BLACK
#define COLOR_TEXT      ST7789_COLOR_WHITE
#define COLOR_SELECTED  ST7789_COLOR_LIGHT_PURPLE
#define COLOR_HIGHLIGHT ST7789_COLOR_RED

#define MAX_FILES 20
#define BTN_UP     15
#define BTN_DOWN   6
#define BTN_OK     4
#define BTN_BACK   7

#include <ctype.h>

static bool is_valid_name(const char *name) {
    if (!name || name[0] == '\0') return false;
    for (int i = 0; i < 255 && name[i]; i++) {
        if (!isprint((unsigned char)name[i])) return false;
    }
    return true;
}

static int load_file_list(char files[][256], int max) {
    DIR *dir = opendir("/sdcard");
    if (!dir) {
        ESP_LOGE("UI_SD", "Erro ao abrir diretório: %d", errno);
        return -1;
    }

    struct dirent *entry = NULL;
    int count = 0;

    while ((entry = readdir(dir)) != NULL && count < max) {
        // Proteção contra nomes inválidos
        if (is_valid_name(entry->d_name)) {
            memset(files[count], 0, 256);
            strncpy(files[count], entry->d_name, 255);
            count++;
        } else {
            ESP_LOGW("UI_SD", "Nome inválido ignorado.");
        }
    }

    if (dir) {
        closedir(dir);
    }

    ESP_LOGI("UI_SD", "Total de arquivos listados: %d", count);
    return count;
}



static void view_file(const char *name) {
    char fullPath[272];
    strcpy(fullPath, "/sdcard/");
    strncat(fullPath, name, sizeof(fullPath) - strlen(fullPath) - 1);

    st7789_fill_screen(COLOR_BG);
    st7789_draw_text(10, 5, "Visualizando:", COLOR_TEXT);
    st7789_draw_text(10, 25, name, COLOR_SELECTED);

    st7789_draw_text(10, 45, fullPath, COLOR_TEXT);

    FILE *f = fopen(fullPath, "r");
    if (!f) {
        st7789_draw_text(10, 80, "Erro ao abrir", COLOR_HIGHLIGHT);
        ESP_LOGE("SD_VIEW", "Erro ao abrir %s (errno = %d)", fullPath, errno);
        vTaskDelay(pdMS_TO_TICKS(1500));
        return;
    }

    char line[64];
    int y = 80;
    while (fgets(line, sizeof(line), f) && y < 220) {
        line[strcspn(line, "\n")] = '\0';
        st7789_draw_text(10, y, line, COLOR_TEXT);
        y += 20;
    }

    fclose(f);
    st7789_draw_text(10, 220, "Pressione BACK", COLOR_SELECTED);

    while (gpio_get_level(BTN_BACK)) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    vTaskDelay(pdMS_TO_TICKS(300));
}

void sd_menu_screen(void) {
    ESP_LOGI("SD_MENU", "Entrando no menu SD");
    st7789_fill_screen(COLOR_BG);
    st7789_set_text_size(2);

    if (!sdcard_is_mounted()) {
        ESP_LOGI("SD_MENU", "Cartão ainda não montado. Tentando montar...");
        if (sdcard_init() != ESP_OK) {
            ESP_LOGE("SD_MENU", "Falha ao montar SD");
            st7789_draw_text(10, 30, "SD Falhou!", COLOR_HIGHLIGHT);
            vTaskDelay(pdMS_TO_TICKS(2000));
            return;
        }
        ESP_LOGI("SD_MENU", "Cartão montado com sucesso");
    }

    sdmmc_card_t *card = hb_sd_get_card();
    if (card) {
        char info[64];
        snprintf(info, sizeof(info), "Modelo: %s", card->cid.name);
        st7789_draw_text(10, 5, info, COLOR_TEXT);
    }

    char files[MAX_FILES][256];
    int fileCount = load_file_list(files, MAX_FILES);
    if (fileCount < 0) {
        st7789_draw_text(10, 40, "Erro ao abrir /sdcard", COLOR_HIGHLIGHT);
        return;
    }

    int selected = 0;
    bool running = true;
    bool needs_redraw = true;

    while (running) {
        if (needs_redraw) {
            st7789_draw_rect(0, 30, 240, 180, COLOR_BG);

            if (fileCount == 0) {
                st7789_draw_text(10, 40, "Nenhum arquivo", COLOR_TEXT);
            } else {
                for (int i = 0; i < fileCount && i < 6; i++) {
                    uint16_t color = (i == selected) ? COLOR_SELECTED : COLOR_TEXT;
                    st7789_draw_text(10, 40 + i * 20, files[i], color);
                }
            }

            needs_redraw = false;
        }

        vTaskDelay(pdMS_TO_TICKS(30));

        if (!gpio_get_level(BTN_UP)) {
            selected = (selected - 1 + fileCount) % fileCount;
            needs_redraw = true;
            vTaskDelay(pdMS_TO_TICKS(200));
        } else if (!gpio_get_level(BTN_DOWN)) {
            selected = (selected + 1) % fileCount;
            needs_redraw = true;
            vTaskDelay(pdMS_TO_TICKS(200));
        } else if (!gpio_get_level(BTN_OK) && fileCount > 0) {
            view_file(files[selected]);
            needs_redraw = true;
        } else if (!gpio_get_level(BTN_BACK)) {
            running = false;
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

    sdcard_unmount();
}