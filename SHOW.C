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

#define KEY_ESC 27
void
maybe_exit(void)
{
	int ch;

	if (!kbhit())
		return;

	ch = getch();

	/* read away function/arrow keys */
	if (ch == 0 || ch == 224) {
		getch();
		return;
	}

	if (ch == KEY_ESC || tolower(ch) == 'q') {
		setmode(MODE_TEXT);
		exit(0);
	}
}

static void
mda_show(struct bitmap *bmp)
{
	WORD row_ofs, col_ofs;
	WORD row, col;

	col_ofs = MDA_WIDTH  / 2 - bmp->width  / 2;
	row_ofs = MDA_HEIGHT / 2 - bmp->height / 2;
	grayscale_dither(bmp, 2);

	mda_set_mode(MDA_GRAPHICS_MODE);
	mda_clear_screen();

	for (row = 0; row < bmp->height; ++row) {
		size_t ofs = row * bmp->width;

		maybe_exit();
		for (col = 0; col < bmp->width; ++col) {
			BYTE Y = bmp->image[ofs + col] >> 7;

			mda_plot(col + col_ofs, row + row_ofs, Y);
		}
	}
}

static void
cga_show(struct bitmap *bmp)
{
	/*
	 * The cga palette is for monochrome monitors, so basically a
	 * grayscale gradient. The darker version of the default color
	 * palette is meant for this usage.
	 */
	BYTE pal[4] = { 0, 2, 1, 3 };
	WORD row_ofs, col_ofs;
	WORD row, col;

	col_ofs = CGA_WIDTH  / 2 - bmp->width  / 2;
	row_ofs = CGA_HEIGHT / 2 - bmp->height / 2;
	grayscale_dither(bmp, 4);

	cga_clear_screen();
	for (row = 0; row < bmp->height; ++row) {
		size_t ofs = row * bmp->width;

		maybe_exit();
		for (col = 0; col < bmp->width; ++col) {
			BYTE Y = bmp->image[ofs + col] >> 6;
			BYTE color = pal[Y];

			cga_plot(col + col_ofs, row + row_ofs, color);
		}
	}
}

static void
ega_show(struct bitmap *bmp)
{
	/*
	 * Custom palette for ega mode tries to have a broad color spectrum.
	 */
	struct rgb pal[] = {
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
	WORD row_ofs, col_ofs;
	WORD row, col;

	col_ofs = EGA_WIDTH  / 2 - bmp->width  / 2;
	row_ofs = EGA_HEIGHT / 2 - bmp->height / 2;

	dither(bmp, pal, 16);
	ega_clear_screen(); /* XXX: resets palette */
	ega_set_palette(pal, 16);

	for (row = 0; row < bmp->height - 1; ++row) {
		size_t ofs = row * bmp->width;

		maybe_exit();
		for (col = 1; col < bmp->width - 1; ++col) {
			BYTE color = bmp->image[ofs + col];

			ega_plot(col + col_ofs, row + row_ofs, color);
		}
	}
}

static void
vga_show(struct bitmap *bmp)
{
	WORD row_ofs, col_ofs;
	WORD row;

	col_ofs = VGA_WIDTH  / 2 - bmp->width  / 2;
	row_ofs = VGA_HEIGHT / 2 - bmp->height / 2;

	vga_clear_screen();
	vga_set_palette(bmp->palette);
	for (row = 0; row < bmp->height; ++row) {
		void *src, *dst;

		maybe_exit();
		src = bmp->image + row * bmp->width;
		dst = vga_vmem_ptr(col_ofs, row + row_ofs);
		memcpy(dst, src, bmp->width);
	}
}


int
main(int argc, char **argv)
{
	struct ffblk ffblk;
	void (*show)(struct bitmap *);
	unsigned int waitms = 5 * 1000;

	if (argc == 2) {
		waitms = atoi(argv[1]);
		if (waitms == 0) {
			fprintf(stderr, "\bInvalid delay.\n");
			return 1;
		}
		waitms *= 1000;
	}

	if (findfirst("*.bmp", &ffblk, 0) == -1) {
		fprintf(stderr, "No images found.\n");
		return 1;
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
		int rc;

		for (rc = findfirst("*.bmp", &ffblk, 0);
		     rc == 0;
		     rc = findnext(&ffblk)) {
			struct bitmap *bmp;
			unsigned int i;

			bmp = bitmap_read(ffblk.ff_name);
			show(bmp);
			bitmap_free(bmp);

			for (i = 0; i < waitms; i += 100) {
				maybe_exit();
				delay(100);
			}
		}
	}
}

