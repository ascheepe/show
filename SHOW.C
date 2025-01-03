#include <dir.h>
#include <dos.h>
#include <conio.h>
#include <ctype.h>
#include <stdlib.h>

#include "bitmap.h"
#include "color.h"
#include "detect.h"
#include "dither.h"
#include "globals.h"
#include "system.h"

#include "mda.h"
#include "cga.h"
#include "ega.h"
#include "vga.h"

#define KEY_ESC 27
int
maybe_exit(void)
{
	int ch;

	if (!kbhit())
		return 0;

	ch = getch();

	/* read away function/arrow keys */
	if (ch == 0 || ch == 224)
		return getch();

	if (ch == KEY_ESC || tolower(ch) == 'q') {
		setmode(MODE_TEXT);
		exit(0);
	}

	return ch;
}

int
main(int argc, char **argv)
{
	WORD waitms = 5 * 1000;
	struct ffblk ffblk;

	if (argc == 2) {
		waitms = atoi(argv[1]);
		if (waitms == 0) {
			fprintf(stderr, "\aInvalid delay.\n");
			return 1;
		}
		waitms *= 1000;
	}

	if (findfirst("*.bmp", &ffblk, 0) == -1) {
		fprintf(stderr, "No images found.\n");
		return 1;
	}

	graphics_mode = detect_graphics();

	switch (graphics_mode) {
	case MDA_GRAPHICS:
		mda_set_mode(MDA_GRAPHICS_MODE);
		plot = mda_plot;
		break;

	case CGA_GRAPHICS:
		setmode(MODE_CGA);
		plot = cga_plot;
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

	for (;;) {
		int r;

		for (
			r = findfirst("*.bmp", &ffblk, 0);
			r == 0;
			r = findnext(&ffblk)
		) {
			WORD i;

			bitmap_show(ffblk.ff_name);

			for (i = 0; i < waitms; i += 100) {
				if (maybe_exit())
					break;
				delay(100);
			}
		}
	}
}
