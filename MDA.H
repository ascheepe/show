#ifndef MDA_H
#define MDA_H

#include "system.h"

#define MDA_WIDTH  320
#define MDA_HEIGHT 200

enum {
	MDA_TEXT_MODE,
	MDA_GRAPHICS_MODE
};

void mda_set_mode(int);
void mda_plot(u16, u16, u8);
void mda_clear_screen(void);

#endif
