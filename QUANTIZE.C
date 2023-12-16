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

#include "system.h"
#include "bitmap.h"
#include "color.h"

/* XXX: this needs a large stack otherwise we run out of it. */
extern unsigned _stklen = 1024 * 62;

/* a private copy of a bitmap to work on */
static struct bitmap *bmp;

/*
 * See which color component has the largest range.
 */
static int
get_max_range(WORD row_start, WORD row_end)
{
	BYTE rmax = 0;
	BYTE gmax = 0;
	BYTE bmax = 0;
	WORD row, col;

	for (row = row_start; row < row_end; ++row) {
		for (col = 0; col < bmp->width; ++col) {
			struct rgb *color;

			color =
			    &bmp->palette[bmp->image[row * bmp->width + col]];
			if (color->r > rmax)
				rmax = color->r;

			if (color->g > gmax)
				gmax = color->g;

			if (color->b > bmax)
				bmax = color->b;

		}
	}

	if (rmax > gmax && rmax > bmax)
		return MAX_RANGE_RED;

	if (gmax > rmax && gmax > bmax)
		return MAX_RANGE_GREEN;

	return MAX_RANGE_BLUE;
}

/*
 * Calculate a running average for the pixel colors between
 * row_start and row_end.
 * Scale color components by 8 to have a low precision fixed-point
 * value which we can round back in the end.
 */
static struct rgb *
get_average_color(int row_start, int row_end)
{
	struct rgb *avg;
	DWORD ravg = 0;
	DWORD gavg = 0;
	DWORD bavg = 0;
	DWORD ncolors = 0;
	WORD row, col;

	for (row = row_start; row < row_end; ++row) {
		for (col = 0; col < bmp->width; ++col) {
			struct rgb *color;

			color =
			    &bmp->palette[bmp->image[row * bmp->width + col]];
			ravg = (color->r * 8 + ncolors * ravg) / (ncolors + 1);
			gavg = (color->g * 8 + ncolors * gavg) / (ncolors + 1);
			bavg = (color->b * 8 + ncolors * bavg) / (ncolors + 1);
			++ncolors;
		}
	}

	avg = xmalloc(sizeof(struct rgb));

	avg->r = (ravg + 4) / 8;
	avg->g = (gavg + 4) / 8;
	avg->b = (bavg + 4) / 8;

	return avg;
}

/*
 * Sort pixels by a color component.
 */
static int
by_red(const void *index_a_ptr, const void *index_b_ptr)
{
	BYTE index_a = *((BYTE *) index_a_ptr);
	BYTE index_b = *((BYTE *) index_b_ptr);
	struct rgb *a = &bmp->palette[index_a];
	struct rgb *b = &bmp->palette[index_b];

	return a->r - b->r;
}

static int
by_green(const void *index_a_ptr, const void *index_b_ptr)
{
	BYTE index_a = *((BYTE *) index_a_ptr);
	BYTE index_b = *((BYTE *) index_b_ptr);
	struct rgb *a = &bmp->palette[index_a];
	struct rgb *b = &bmp->palette[index_b];

	return a->g - b->g;
}

static int
by_blue(const void *index_a_ptr, const void *index_b_ptr)
{
	BYTE index_a = *((BYTE *) index_a_ptr);
	BYTE index_b = *((BYTE *) index_b_ptr);
	struct rgb *a = &bmp->palette[index_a];
	struct rgb *b = &bmp->palette[index_b];

	return a->b - b->b;
}

/*
 * Do a median-cut to generate an optimal palette. This is
 * very slow with regard to sorting all the pixels under
 * Turbo C.
 *
 * The selected colors are ok but further mapping to a
 * fixed palette makes some pretty far off, this needs
 * more work.
 */
static void
median_cut(WORD row_start, WORD row_end, int ncuts,
    struct rgb **palette, int *ncolors)
{
	BYTE *img_off;
	size_t img_len;
	int max_range;
	int median;

	printf("Quantizing\r");

	/*
	 * We are done for this bucket, take the average color
	 * and add it to the palette.
	 */
	if (ncuts == 0) {
		struct rgb *avg;
		struct rgb *palette_entry;
		int palidx;

		palidx = *ncolors;
		++(*ncolors);
		*palette = xrealloc(*palette, sizeof(struct rgb) * *ncolors);
		avg = get_average_color(row_start, row_end);
		palette_entry = &(*palette)[palidx];
		palette_entry->r = avg->r;
		palette_entry->g = avg->g;
		palette_entry->b = avg->b;
		free(avg);
		return;
	}

	max_range = get_max_range(row_start, row_end);

	img_off = bmp->image + bmp->width * row_start;
	img_len = bmp->width * (row_end - row_start);

	switch (max_range) {
	case MAX_RANGE_RED:
		qsort(img_off, img_len, 1, by_red);
		break;

	case MAX_RANGE_GREEN:
		qsort(img_off, img_len, 1, by_green);
		break;

	case MAX_RANGE_BLUE:
		qsort(img_off, img_len, 1, by_blue);
		break;

	default:
		xerror("median_cut: illegal max_range.");
	}

	median = (row_start + row_end) / 2;

	median_cut(row_start, median, ncuts - 1, palette, ncolors);
	median_cut(median, row_end, ncuts - 1, palette, ncolors);
}

/*
 * Median cut driver.
 */
struct rgb *
quantize(struct bitmap *original_bmp, int ncuts)
{
	struct rgb *palette = NULL;
	int ncolors = 0;
	int i;

	bmp = bitmap_copy(original_bmp);
	median_cut(0, bmp->height, ncuts, &palette, &ncolors);
	bitmap_free(bmp);
	bmp = NULL;

	return palette;
}

