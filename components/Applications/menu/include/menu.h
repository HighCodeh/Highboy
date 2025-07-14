#ifndef MENU_H
#define MENU_H

#include <stdbool.h>

typedef enum {
    STATE_HOME,
    STATE_MENU
} app_state_t;


void menu_task(void *pvParameters);

#endif 
