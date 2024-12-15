#ifndef EGA_H
#define EGA_H

#include "system.h"
#include "color.h"

#define EGA_WIDTH 640
#define EGA_HEIGHT 350

void ega_plot(int, int, int);
void ega_plot(int, int, int);
void ega_clear_screen(void);
void ega_set_palette(struct rgb *, int);
#endif
