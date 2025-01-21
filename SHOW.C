#include "bitmap.h"
#include "compat.h"
#include "detect.h"
#include "dither.h"
#include "globals.h"
#include "system.h"

#include "mda.h"
#include "cga.h"
#include "cplus.h"
#include "ega.h"
#include "vga.h"

int
main(int argc, char **argv)
{
	graphics_mode = detect_graphics();

	switch (graphics_mode) {
	case MDA_GRAPHICS:
		mda_set_mode(MDA_GRAPHICS_MODE);
		plot = mda_plot;
		break;
	case CGA_GRAPHICS:
		setmode(MODE_CGA);
		if (is_cplus()) {
			cplus_init();
			graphics_mode = CPLUS_GRAPHICS;
			plot = cplus_plot;
		} else {
			plot = cga_plot;
		}
		break;
	case EGA_GRAPHICS:
		setmode(MODE_EGA);
		plot = ega_plot;
		break;
	case VGA_GRAPHICS:
		setmode(MODE_VGA);
		plot = vga_plot;
		break;
	}

	if (argc > 1) {
		for (++argv; *argv != NULL; ++argv)
			bitmap_show(*argv);
	} else {
		for (;;)
			foreach_bmp(bitmap_show);
	}

	setmode(MODE_TXT);
	return 0;
}
