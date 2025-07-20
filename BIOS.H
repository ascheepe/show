#ifndef BIOS_H
#define BIOS_H

#include "system.h"

#define BIOS_WIDTH 320
#define BIOS_HEIGHT 200

void bios_plot(WORD, WORD, BYTE);
void bios_clear_screen(void);

#endif
