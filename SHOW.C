#include <dos.h>
#include <conio.h>
#include <ctype.h>
#include <stdlib.h>

#include "bitmap.h"
#include "color.h"
#include "compat.h"
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

void
showfile(char *filename)
{
	bitmap_show(filename);
	for (;;)
		if (maybe_exit())
			break;
}

int
main(int argc, char **argv)
{
	if (argc == 2) {
		if (!file_exists(argv[1])) {
			fprintf(stderr, "Can't open '%s'.\n", argv[1]);
			return 1;
		}
	} else if (!bmp_present()) {
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

	if (argc == 2)
		showfile(argv[1]);
	else
		for (;;)
			foreach_bmp(showfile);

	return 0;
}
