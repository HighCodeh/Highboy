// main/tusb_desc.h

#ifndef TUSB_DESC_H
#define TUSB_DESC_H


#include "tinyusb.h"
// --- Constantes Públicas ---
// Define um nome para a nossa única interface HID (Teclado)
#define ITF_NUM_HID   0

// --- Declarações Externas ---
// Informa a outros arquivos que essas variáveis existem em algum lugar (no tusb_desc.c)
void busb_init(void);

#endif // TUSB_DESC_H
