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

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system.h"
#include "vector.h"
#include "color.h"

struct vector *
palette_to_vector(BYTE *palette, int size)
{
	struct vector *ret;
	size_t i;

	ret = vector_new();

	for (i = 0; i < size; ++i) {
		struct color *color;
		int offset = i * 3;

		color = xmalloc(sizeof(*color));
		color->r = palette[offset + 0];
		color->g = palette[offset + 1];
		color->b = palette[offset + 2];

		vector_add(ret, color);
	}

	return ret;
}


/* which part has the maximum val, r, g or b? */
static int
max_color(struct vector *palette)
{
	BYTE rmax = 0, gmax = 0, bmax = 0;
	size_t i;

	for (i = 0; i < palette->size; ++i) {
		struct color *color = palette->items[i];

		if (color->r > rmax)
			rmax = color->r;

		if (color->g > gmax)
			gmax = color->g;

		if (color->b > bmax)
			bmax = color->b;
	}

	if (rmax > gmax && rmax > bmax)
		return MAX_COLOR_RED;

	if (gmax > rmax && gmax > bmax)
		return MAX_COLOR_GREEN;

	return MAX_COLOR_BLUE;
}

static int
sort_by_red(const void *color_a, const void *color_b)
{
	struct color *a = *((struct color **) color_a);
	struct color *b = *((struct color **) color_b);

	return a->r - b->r;
}

static int
sort_by_green(const void *color_a, const void *color_b)
{
	struct color *a = *((struct color **) color_a);
	struct color *b = *((struct color **) color_b);

	return a->g - b->g;
}

static int
sort_by_blue(const void *color_a, const void *color_b)
{
	struct color *a = *((struct color **) color_a);
	struct color *b = *((struct color **) color_b);

	return a->b - b->b;
}

static void
sort_palette(struct vector *palette, int by)
{
	int (*compare)(const void *, const void *) = NULL;

	switch (by) {
	case MAX_COLOR_RED:
		compare = sort_by_red;
		break;

	case MAX_COLOR_GREEN:
		compare = sort_by_green;
		break;

	case MAX_COLOR_BLUE:
		compare = sort_by_blue;
		break;

	default:
		xerror("sort_palette: unknown sort function.");
	}

	qsort(palette->items, palette->size, sizeof(void *), compare);
}

/* average color of palette */
static struct color *
palette_average(struct vector *palette)
{
	struct color *avg;
	DWORD rsum = 0, gsum = 0, bsum = 0;
	size_t i;

	avg = xmalloc(sizeof(*avg));

	for (i = 0; i < palette->size; ++i) {
		struct color *color = palette->items[i];

		rsum += color->r;
		gsum += color->g;
		bsum += color->b;
	}

	avg->r = rsum / palette->size;
	avg->g = gsum / palette->size;
	avg->b = bsum / palette->size;

	return avg;
}

void
median_cut(struct vector *palette, int ncuts, struct vector *reduced)
{
	struct vector *top = NULL, *bottom = NULL;
	size_t i, median;

	/*
	 * if done add the average color of the bucket
	 * to the reduced palette.
	 */
	if (ncuts == 0) {
		struct color *avg;

		avg = palette_average(palette);
		vector_add(reduced, avg);
		return;
	}

	/* sort palette by highest color val of r,g,b */
	sort_palette(palette, max_color(palette));

	/* split into top and bottom part */
	median = palette->size >> 1;
	bottom = vector_new();
	top = vector_new();

	for (i = 0; i < median; ++i)
		vector_add(bottom, palette->items[i]);

	for (i = median; i < palette->size; ++i)
		vector_add(top, palette->items[i]);

	/* repeat for the two created buckets */
	median_cut(bottom, ncuts - 1, reduced);
	median_cut(top, ncuts - 1, reduced);

	vector_free(bottom);
	vector_free(top);
}

static void
print_binary(FILE *f, BYTE value)
{
	int bit;

	for (bit = 0; bit < 8; ++bit) {
		int mask = (1 << (7 - bit));

		if (value & mask)
			fputc('1', f);
		else
			fputc('0', f);
	}
}

void
write_palette(struct vector *palette, char *filename)
{
	FILE *f;
	size_t i;

	if ((f = fopen(filename, "w")) == NULL)
		xerror("write_palette: can't open file.");

	for (i = 0; i < palette->size; ++i) {
		struct color *color = palette->items[i];
		BYTE ega_r = color->r >> 6;
		BYTE ega_g = color->g >> 6;
		BYTE ega_b = color->b >> 6;

		fprintf(f,
		    "#%02x%02x%02x => #%02x%02x%02x => %d %d %d => %02x => ",
		    color->r, color->g, color->b,
		    ega_r * 85, ega_g * 85, ega_b * 85,
		    ega_r, ega_g, ega_b, ega_make_color(color));
		print_binary(f, ega_make_color(color));
		fputc('\n', f);
	}

	fclose(f);
}
