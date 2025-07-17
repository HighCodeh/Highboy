// menu_generic.h
#pragma once
#include <stdint.h>

typedef struct {
    const char* label;
    const uint16_t* icon;
    void (*action)(void);
} MenuItem;

typedef struct {
    const char* title;
    MenuItem* items;
    int item_count;
} Menu;

void show_menu(Menu* menu);