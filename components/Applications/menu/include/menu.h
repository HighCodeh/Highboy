#ifndef MENU_H
#define MENU_H

#include <stdbool.h>

typedef enum {
    STATE_HOME,
    STATE_MENU
} app_state_t;

static app_state_t current_state = STATE_HOME;

void menu_init(void);
void showMenu(void);
void handleMenuControls(void);
static int lerp(int start, int end, float t);
static void drawMenuItem(int menuIndex, int posY, bool isSelected);
static void drawScrollBar(void);
static void executeMenuItem(int index);


void menu_task(void *pvParameters);

#endif // MENU_H
