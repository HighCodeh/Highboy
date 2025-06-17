#include "sub_menu.h"
#include "st7789.h"
#include "ir_transmitter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <stdio.h>

void show_lg_menu(void);

// === Comandos LG ===
void lg_power(void)       { ir_send_nec_raw(0x20DF, 0x10); }
void lg_energy(void)      { ir_send_nec_raw(0x20DF, 0xA9); }
void lg_av_mode(void)     { ir_send_nec_raw(0x20DF, 0x0C); }
void lg_input(void)       { ir_send_nec_raw(0x20DF, 0xD0); }
void lg_tv_radio(void)    { ir_send_nec_raw(0x20DF, 0x0F); }

void lg_1(void)           { ir_send_nec_raw(0x20DF, 0x88); }
void lg_2(void)           { ir_send_nec_raw(0x20DF, 0x48); }
void lg_3(void)           { ir_send_nec_raw(0x20DF, 0xC8); }
void lg_4(void)           { ir_send_nec_raw(0x20DF, 0x28); }
void lg_5(void)           { ir_send_nec_raw(0x20DF, 0xA8); }
void lg_6(void)           { ir_send_nec_raw(0x20DF, 0x68); }
void lg_7(void)           { ir_send_nec_raw(0x20DF, 0xE8); }
void lg_8(void)           { ir_send_nec_raw(0x20DF, 0x18); }
void lg_9(void)           { ir_send_nec_raw(0x20DF, 0x98); }
void lg_0(void)           { ir_send_nec_raw(0x20DF, 0x08); }

void lg_list(void)        { ir_send_nec_raw(0x20DF, 0xCA); }
void lg_quick_view(void)  { ir_send_nec_raw(0x20DF, 0x58); }

void lg_volume_up(void)   { ir_send_nec_raw(0x20DF, 0x40); }
void lg_volume_down(void) { ir_send_nec_raw(0x20DF, 0xC0); }

void lg_channel_up(void)  { ir_send_nec_raw(0x20DF, 0x00); }
void lg_channel_down(void){ ir_send_nec_raw(0x20DF, 0x80); }

void lg_guide(void)       { ir_send_nec_raw(0x20DF, 0xD5); }
void lg_home(void)        { ir_send_nec_raw(0x20DF, 0xC2); }
void lg_fav(void)         { ir_send_nec_raw(0x20DF, 0x78); }
void lg_ratio(void)       { ir_send_nec_raw(0x20DF, 0x9E); }
void lg_mute(void)        { ir_send_nec_raw(0x20DF, 0x90); }

void lg_up(void)          { ir_send_nec_raw(0x20DF, 0x02); }
void lg_down(void)        { ir_send_nec_raw(0x20DF, 0x82); }
void lg_left(void)        { ir_send_nec_raw(0x20DF, 0xE0); }
void lg_right(void)       { ir_send_nec_raw(0x20DF, 0x60); }

void lg_ok(void)          { ir_send_nec_raw(0x20DF, 0x22); }
void lg_back(void)        { ir_send_nec_raw(0x20DF, 0x14); }
void lg_info(void)        { ir_send_nec_raw(0x20DF, 0x55); }
void lg_exit(void)        { ir_send_nec_raw(0x20DF, 0xDA); }

void lg_red(void)         { ir_send_nec_raw(0x20DF, 0x4E); }
void lg_green(void)       { ir_send_nec_raw(0x20DF, 0x8E); }
void lg_yellow(void)      { ir_send_nec_raw(0x20DF, 0xC6); }
void lg_blue(void)        { ir_send_nec_raw(0x20DF, 0x86); }

void lg_text(void)        { ir_send_nec_raw(0x20DF, 0x04); }
void lg_t_opt(void)       { ir_send_nec_raw(0x20DF, 0x84); }
void lg_subtitle(void)    { ir_send_nec_raw(0x20DF, 0x9C); }

void lg_stop(void)        { ir_send_nec_raw(0x20DF, 0x8D); }
void lg_play(void)        { ir_send_nec_raw(0x20DF, 0x0D); }
void lg_pause(void)       { ir_send_nec_raw(0x20DF, 0x5D); }
void lg_rewind(void)      { ir_send_nec_raw(0x20DF, 0xF1); }
void lg_forward(void)     { ir_send_nec_raw(0x20DF, 0x71); }

void lg_ad(void)          { ir_send_nec_raw(0x20DF, 0x89); }

// === Menu da LG ===
void show_lg_menu(void) {
    MenuItem lgItems[] = {
        { "Power", NULL, lg_power },
        { "Energy", NULL, lg_energy },
        { "AV Mode", NULL, lg_av_mode },
        { "Input", NULL, lg_input },
        { "TV/Radio", NULL, lg_tv_radio },
        { "1", NULL, lg_1 },
        { "2", NULL, lg_2 },
        { "3", NULL, lg_3 },
        { "4", NULL, lg_4 },
        { "5", NULL, lg_5 },
        { "6", NULL, lg_6 },
        { "7", NULL, lg_7 },
        { "8", NULL, lg_8 },
        { "9", NULL, lg_9 },
        { "0", NULL, lg_0 },
        { "List", NULL, lg_list },
        { "Quick View", NULL, lg_quick_view },
        { "Vol +", NULL, lg_volume_up },
        { "Vol -", NULL, lg_volume_down },
        { "Ch +", NULL, lg_channel_up },
        { "Ch -", NULL, lg_channel_down },
        { "Guide", NULL, lg_guide },
        { "Home", NULL, lg_home },
        { "Fav", NULL, lg_fav },
        { "Ratio", NULL, lg_ratio },
        { "Mute", NULL, lg_mute },
        { "Up", NULL, lg_up },
        { "Down", NULL, lg_down },
        { "Left", NULL, lg_left },
        { "Right", NULL, lg_right },
        { "OK", NULL, lg_ok },
        { "Back", NULL, lg_back },
        { "Info", NULL, lg_info },
        { "Exit", NULL, lg_exit },
        { "Red", NULL, lg_red },
        { "Green", NULL, lg_green },
        { "Yellow", NULL, lg_yellow },
        { "Blue", NULL, lg_blue },
        { "Text", NULL, lg_text },
        { "T.Opt", NULL, lg_t_opt },
        { "Subtitle", NULL, lg_subtitle },
        { "Stop", NULL, lg_stop },
        { "Play", NULL, lg_play },
        { "Pause", NULL, lg_pause },
        { "Rewind", NULL, lg_rewind },
        { "Forward", NULL, lg_forward },
        { "AD", NULL, lg_ad },
    };
    show_menu(lgItems, sizeof(lgItems) / sizeof(MenuItem), "LG TV");
    handle_menu_controls();
}
