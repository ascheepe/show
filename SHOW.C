#include "compat.h"
#include "detect.h"
#include "dither.h"
#include "globals.h"
#include "pcx.h"
#include "system.h"

#include "mda.h"
#include "cga.h"
#include "cplus.h"
#include "tga.h"
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
		if (is_cplus()) {
			setmode(MODE_CGA);
			cplus_init();
			graphics_mode = CPLUS_GRAPHICS;
			plot = cplus_plot;
		} else {
			setmode(MODE_CGA);
			plot = cga_plot;
		}
		break;
	case TGA_GRAPHICS:
		setmode(MODE_TGA);
		plot = tga_plot;
		break;
	case EGA_GRAPHICS:
		setmode(MODE_EGA);
		plot = ega_plot;
		break;
	case VGA_GRAPHICS:
		setmode(MODE_VGA);
		plot = vga_plot;
		break;
	default:
	case GRAPHICS_ERROR:
		die("Can't determine graphics card.");
	}

	if (argc > 1) {
		for (++argv; *argv != NULL; ++argv)
			pcx_show(*argv);
	} else {
		for (;;)
			foreach_pcx(pcx_show);
	}

	setmode(MODE_TXT);
	return 0;
}
