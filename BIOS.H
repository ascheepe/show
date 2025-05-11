#ifndef BIOS_H
#define BIOS_H

#include "system.h"

#define BIOS_WIDTH 320
#define BIOS_HEIGHT 200

void bios_plot(u16, u16, u8);
void bios_clear_screen(void);

#endif
