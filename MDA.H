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
void mda_plot(WORD, WORD, BYTE);
void mda_plot_scaled(WORD, WORD, BYTE);
void mda_clear_screen(void);

#endif
