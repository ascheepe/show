#ifndef CGA_H
#define CGA_H

#include "system.h"

#define CGA_WIDTH 320
#define CGA_HEIGHT 200

void cga_plot(WORD, BYTE, int);
void cga_clear_screen(void);

#endif
