#ifndef EGA_H
#define EGA_H

#include "globals.h"
#include "system.h"

#define EGA_WIDTH 320
#define EGA_HEIGHT 200

void ega_plot(int, int, int);
void ega_plot(int, int, int);
void ega_clear_screen(void);
void ega_set_palette(struct rgb *, int);
#endif
