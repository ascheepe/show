/*
 * Copyright (c) 2020 Axel Scheepers
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
#include <time.h>
#include <conio.h>
#include <dir.h>
#include <dos.h>

#include "system.h"
#include "array.h"
#include "color.h"
#include "bitmap.h"
#include "detect.h"
#include "dither.h"
#include "mda.h"
#include "cga.h"
#include "ega.h"
#include "vga.h"

static enum graphics_type graphics_type;
static void (*show)(char *);

static void
mda_show(char *filename)
{
	struct bitmap *bmp;
	int row, roff, col, coff;

	bmp = bitmap_read(filename);
	coff = 360 - (bmp->width >> 1);
	roff = 174 - (bmp->height >> 1);

	convert_to_grayscale(bmp);
	dither(bmp, 2);
	mda_clear_screen();

	for (row = 0; row < bmp->height; ++row) {
		for (col = 0; col < bmp->width; ++col) {
			BYTE y = bmp->image[row * bmp->width + col] >> 7;

			mda_plot(col + coff, row + roff, y);
		}
	}

	bitmap_free(bmp);
}

static void
cga_show(char *filename)
{
	struct bitmap *bmp;
	int row, roff, col, coff;

	bmp = bitmap_read(filename);
	coff = 160 - (bmp->width >> 1);
	roff = 100 - (bmp->height >> 1);

	convert_to_grayscale(bmp);
	dither(bmp, 4);
	cga_clear_screen();

	for (row = 0; row < bmp->height; ++row) {
		for (col = 0; col < bmp->width; ++col) {
			BYTE pal[4] = { 0, 2, 1, 3 };
			BYTE y = bmp->image[row * bmp->width + col] >> 6;

			cga_plot(col + coff, row + roff, pal[y]);
		}
	}

	bitmap_free(bmp);
}

static void
ega_show(char *filename)
{
	struct bitmap *bmp;
	int row, roff, col, coff;
	struct array *palette, *reduced;

	bmp = bitmap_read(filename);
	palette = palette_to_array(bmp->palette, bmp->ncolors);
	coff = 320 - (bmp->width >> 1);
	roff = 175 - (bmp->height >> 1);

	reduced = array_new();
	median_cut(palette, 4, reduced);
	ega_clear_screen();
	ega_set_palette(reduced);

	for (row = 0; row < bmp->height; ++row) {
		for (col = 0; col < bmp->width; ++col) {
			int offset = bmp->image[row * bmp->width + col];
			struct color *color = palette->items[offset];
			DWORD maxdist = 0xffffffffUL;
			int match;
			int i;

			for (i = 0; i < reduced->size; ++i) {
				struct color *new_color =
				    reduced->items[i];
				DWORD rdiff = color->r - new_color->r;
				DWORD gdiff = color->g - new_color->g;
				DWORD bdiff = color->b - new_color->b;
				DWORD dist = SQR(rdiff) + SQR(gdiff) +
				    SQR(bdiff);

				if (dist < maxdist) {
					maxdist = dist;
					match = i;
				}
			}

			ega_plot(col + coff, row + roff, match);
		}
	}

	array_for_each(palette, free);
	array_free(palette);
	array_for_each(reduced, free);
	array_free(reduced);
	bitmap_free(bmp);
}

static void
vga_show(char *filename)
{
	struct bitmap *bmp;
	int row, roff, coff;

	bmp = bitmap_read(filename);
	coff = 160 - (bmp->width >> 1);
	roff = 100 - (bmp->height >> 1);

	vga_clear_screen();
	vga_set_palette(bmp->palette);
	for (row = 0; row < bmp->height; ++row) {
		BYTE *src = bmp->image + row * bmp->width;
		BYTE *dst = vga_memory + VGA_MEM_OFFSET(coff,
		    row + roff);

		memcpy(dst, src, bmp->width);
	}

	bitmap_free(bmp);
}

#define KEY_ESC 27
static int
next_or_exit(void)
{
	int key_pressed = false;

	if (kbhit()) {
		key_pressed = true;
		switch (getch()) {
		case 'q':
		case 'Q':
		case KEY_ESC:
			set_mode(MODE_TEXT);
			exit(EXIT_SUCCESS);

			/* read away special key */
		case 0:
		case 224:
			getch();
			break;
		}
	}

	return key_pressed;
}

#define DEFAULT_WAIT_MSEC 10000
#define DELAY 100

static int
slideshow(int wait_msec)
{
	struct ffblk ffblk;
	int has_images;
	int error;

	has_images = false;
	for (error = findfirst("*.bmp", &ffblk, 0);
	    error == 0;
	    error = findnext(&ffblk)) {
		int total_delays = wait_msec / DELAY;
		int ndelays = 0;

		if (ffblk.ff_attrib & FA_DIREC)
			continue;

		has_images = true;
		show(ffblk.ff_name);
		while (!next_or_exit() && (ndelays < total_delays)) {
			delay(DELAY);
			++ndelays;
		}
	}

	return has_images;
}

int
main(int argc, char *argv[])
{
	int wait_msec = DEFAULT_WAIT_MSEC;

	g_show_progress = true;
	graphics_type = detect_graphics();

	switch (graphics_type) {
	case MDA_GRAPHICS:
		show = mda_show;
		g_show_progress = false;
		mda_set_graphics_mode(1);
		mda_clear_screen();
		break;

	case CGA_GRAPHICS:
		show = cga_show;
		set_mode(MODE_CGA2);
		break;

	case EGA_GRAPHICS:
		show = ega_show;
		set_mode(MODE_EGA);
		break;

	case VGA_GRAPHICS:
		show = vga_show;
		g_show_progress = false;
		set_mode(MODE_VGA);
		break;

	default:
		xerror("Error detecting graphics card.");
	}

	/*
	 * If we have an argument it's either a file to show
	 * or a delay for a slideshow (and the images will be
	 * read from the current directory).
	 */
	if (argc == 2) {
		if (file_exists(argv[1])) {
			show(argv[1]);
			while (!next_or_exit());
		} else {
			wait_msec = atoi(argv[1]) * 1000;

			if (wait_msec <= 0)
				wait_msec = DEFAULT_WAIT_MSEC;
		}
	}

	while (slideshow(wait_msec));

	set_mode(MODE_TEXT);
	xerror("No images found.");
}

