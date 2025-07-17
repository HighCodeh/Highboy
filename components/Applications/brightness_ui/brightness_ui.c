#include "brightness_ui.h"
#include "backlight.h"
#include "st7789.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "pin_def.h"

#define BRIGHTNESS_MIN 1
#define BRIGHTNESS_MAX 255
#define BRIGHTNESS_STEP 51 // 20%

static void draw_brightness_ui(uint8_t brightness)
{
    st7789_fill_screen_fb(ST7789_COLOR_BLACK);

    const int BAR_WIDTH = 40;
    const int BAR_HEIGHT_MAX = 175;
    const int BAR_X = 161;
    const int BAR_Y = 43;
    const int BORDER = 2;

    int fillHeight = (brightness * BAR_HEIGHT_MAX) / 255;
    int fillY = BAR_Y + (BAR_HEIGHT_MAX - fillHeight);

    st7789_fill_rect_fb(BAR_X - BORDER, BAR_Y - BORDER, BAR_WIDTH + 2 * BORDER, BAR_HEIGHT_MAX + 2 * BORDER, ST7789_COLOR_BLACK);
    st7789_draw_rect_fb(BAR_X, BAR_Y, BAR_WIDTH, BAR_HEIGHT_MAX, ST7789_COLOR_WHITE);
    st7789_fill_rect_fb(BAR_X + BORDER, fillY, BAR_WIDTH - 2 * BORDER, fillHeight, ST7789_COLOR_PURPLE);

    int percent = (brightness * 100) / 255;
    char texto[8];
    snprintf(texto, sizeof(texto), "%d%%", percent);
    st7789_draw_text_fb(58, 90, texto, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);

    st7789_draw_text_fb(85, 6, "BRILHO", ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
    st7789_draw_rect_fb(54, 69, 56, 68, 0xFFFF);

    st7789_draw_round_rect_fb(45, 54, 74, 157, 9, 0xFFFF);
    st7789_draw_round_rect_fb(0, 0, 240, 26, 3, 0xFFFF);
    st7789_draw_circle_fb(78, 173, 19, 0xFFFF);
    st7789_draw_circle_fb(99, 150, 4, 0xFFFF);

    st7789_flush();
}

static bool button_pressed(gpio_num_t pin)
{
    return gpio_get_level(pin) == 0;
}

void show_brightness_screen(void)
{
    uint8_t brightness = backlight_get_brightness();
    draw_brightness_ui(brightness);

    vTaskDelay(pdMS_TO_TICKS(250));

    while (1)
    {
        bool changed = false;
        if (button_pressed(BTN_UP))
        {
            if (brightness <= BRIGHTNESS_MAX - BRIGHTNESS_STEP)
            {
                brightness += BRIGHTNESS_STEP;
            }
            else
            {
                brightness = BRIGHTNESS_MAX;
            }
            changed = true;
        }

        if (button_pressed(BTN_DOWN))
        {
            if (brightness >= BRIGHTNESS_MIN + BRIGHTNESS_STEP)
            {
                brightness -= BRIGHTNESS_STEP;
            }
            else
            {
                brightness = BRIGHTNESS_MIN;
            }
            changed = true;
        }

        if (changed)
        {
            backlight_set_brightness(brightness);
            draw_brightness_ui(brightness);
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        if (button_pressed(BTN_BACK))
        {
            vTaskDelay(pdMS_TO_TICKS(200));
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}