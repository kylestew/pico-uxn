#ifndef UXN_H
#define UXN_H

#include <stdint.h>

typedef signed char Sint8;
typedef unsigned char Uint8;
typedef unsigned short Uint16;

extern uint8_t ram[0x10000], dev[0x100], ptr[2], stk[2][0x100];

Uint8 emu_dei(Uint8 port);
void emu_deo(Uint8 port, Uint8 value);

unsigned int uxn_eval(unsigned short pc);

#endif
