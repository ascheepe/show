#ifndef MDA_H
#define MDA_H

#include "system.h"

#define MDA_WIDTH  720
#define MDA_HEIGHT 348

enum {
	MDA_TEXT_MODE,
	MDA_GRAPHICS_MODE
};

void mda_set_mode(int);
void mda_plot(int, int, int);
void mda_clear_screen(void);

#endif
