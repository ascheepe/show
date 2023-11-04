/*
 * Copyright (c) 2020-2022 Axel Scheepers
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <conio.h>
#include <dir.h>
#include <dos.h>

#include "system.h"
#include "color.h"
#include "bitmap.h"
#include "detect.h"
#include "dither.h"
#include "quantize.h"
#include "mda.h"
#include "cga.h"
#include "ega.h"
#include "vga.h"

static void
mda_show(struct bitmap *bmp)
{
	int row_off, col_off;
	int row, col;

	col_off = MDA_WIDTH / 2 - bmp->width / 2;
	row_off = MDA_HEIGHT / 2 - bmp->height / 2;
	grayscale_dither(bmp, 2);

	mda_set_mode(MDA_GRAPHICS_MODE);
	mda_clear_screen();

	for (row = 0; row < bmp->height; ++row) {
		for (col = 0; col < bmp->width; ++col) {
			BYTE luma = bmp->image[row * bmp->width + col] >> 7;

			mda_plot(col + col_off, row + row_off, luma);
		}
	}
}

static void
cga_show(struct bitmap *bmp)
{
	/*
	 * The cga palette is for monochrome monitors, so basically a grayscale
	 * gradient. The darker version of the default color palette is meant for
	 * this usage.
	 */
	BYTE pal[4] = { 0, 2, 1, 3 };
	int row_off, col_off;
	int row, col;

	col_off = CGA_WIDTH / 2 - bmp->width / 2;
	row_off = CGA_HEIGHT / 2 - bmp->height / 2;
	grayscale_dither(bmp, 4);

	cga_clear_screen();
	for (row = 0; row < bmp->height; ++row) {
		for (col = 0; col < bmp->width; ++col) {
			BYTE luma = bmp->image[row * bmp->width + col] >> 6;
			BYTE color = pal[luma];

			cga_plot(col + col_off, row + row_off, color);
		}
	}
}

/*
 * Custom palette for ega mode tries to have a broad color spectrum.
 */
static struct color ega_palette[] = {
	{ 0x00, 0x00, 0x00 },
	{ 0x55, 0x55, 0x55 },
	{ 0xAA, 0xAA, 0xAA },
	{ 0xFF, 0xFF, 0xFF },
	{ 0x55, 0x00, 0x00 },
	{ 0xAA, 0x00, 0x00 },
	{ 0xFF, 0x00, 0x00 },
	{ 0x00, 0x55, 0x00 },
	{ 0x00, 0xAA, 0x00 },
	{ 0x00, 0xFF, 0x00 },
	{ 0x00, 0x00, 0xAA },
	{ 0x00, 0x00, 0xFF },
	{ 0xFF, 0xAA, 0x00 },
	{ 0xFF, 0xFF, 0x00 },
	{ 0xFF, 0x00, 0xFF },
	{ 0x00, 0xFF, 0xFF },
};

static void
ega_show(struct bitmap *bmp)
{
	int row_off, col_off;
	int row, col;

	col_off = EGA_WIDTH / 2 - bmp->width / 2;
	row_off = EGA_HEIGHT / 2 - bmp->height / 2;

	ega_clear_screen(); /* XXX: resets palette */
	ega_set_palette(ega_palette, 16);
	dither(bmp, ega_palette, 16);

	for (row = 0; row < bmp->height - 1; ++row) {
		for (col = 1; col < bmp->width - 1; ++col) {
			BYTE color = bmp->image[row * bmp->width + col];

			ega_plot(col + col_off, row + row_off, color);
		}
	}
}

static void
vga_show(struct bitmap *bmp)
{
	int row_off, col_off;
	int row;

	col_off = VGA_WIDTH / 2 - bmp->width / 2;
	row_off = VGA_HEIGHT / 2 - bmp->height / 2;

	vga_clear_screen();
	vga_set_palette(bmp->palette);
	for (row = 0; row < bmp->height; ++row)
		memcpy(vga_vmem_ptr(col_off, row + row_off),
		    bmp->image + row * bmp->width, bmp->width);
}

#define KEY_ESC 27
#define NO_KEY -1
static int
getkey(void)
{
	int ch = NO_KEY;

	if (kbhit()) {
		ch = getch();

		/* read away special key */
		if (ch == 0 || ch == 224) {
			getch();
			ch = NO_KEY;
		}
	}

	return ch;
}

int
main(int argc, char **argv)
{
	struct ffblk ffblk;
	struct bitmap *bmp;
	void (*show)(struct bitmap *);
	unsigned int waitms = 5 * 1000;
	int status;

	if (argc == 2) {
		waitms = atoi(argv[1]);
		if (waitms <= 0) {
			fprintf(stderr, "\binvalid delay.\n");
			return 1;
		}
		waitms *= 1000;
	}

	switch (detect_graphics()) {
	case MDA_GRAPHICS:
		setmode(MDA_GRAPHICS_MODE);
		show = mda_show;
		break;

	case CGA_GRAPHICS:
		setmode(MODE_CGA);
		show = cga_show;
		break;

	case EGA_GRAPHICS:
		setmode(MODE_EGA);
		show = ega_show;
		break;

	case VGA_GRAPHICS:
		setmode(MODE_VGA);
		show = vga_show;
		break;
	}

	for (;;) {
		for (status = findfirst("*.bmp", &ffblk, 0);
		     status == 0;
		     status = findnext(&ffblk)) {
			unsigned int i;

			bmp = bitmap_read(ffblk.ff_name);
			show(bmp);
			bitmap_free(bmp);

			for (i = 0; i < waitms; i += 100) {
				int ch;

				ch = tolower(getkey());
				if (ch == 'q' || ch == KEY_ESC) {
					setmode(MODE_TEXT);
					return 0;
				}
				delay(100);
			}
		}
	}
}
