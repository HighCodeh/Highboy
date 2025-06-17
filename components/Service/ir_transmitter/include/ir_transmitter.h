
#ifndef IR_TRANSMITTER_H
#define IR_TRANSMITTER_H

#include <stdint.h>

void ir_init(void);
void ir_send_nec_raw(uint16_t address, uint8_t command);

#endif
