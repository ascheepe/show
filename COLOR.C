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
#include "array.h"
#include "color.h"

struct array *
palette_to_array(BYTE *palette, int palette_size)
{
	struct array *ret;
	size_t i;

	ret = array_new();
	for (i = 0; i < palette_size; ++i) {
		struct color *color;
		int offset = i * 3;

		color = xmalloc(sizeof(struct color));
		color->r = palette[offset + 0];
		color->g = palette[offset + 1];
		color->b = palette[offset + 2];

		array_add(ret, color);
	}

	return ret;
}


/* which part has the maximum val, r, g or b? */
static int
max_color(struct array *palette)
{
	size_t i;
	BYTE rmax, gmax, bmax;

	rmax = gmax = bmax = 0;
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

	if (a->r < b->r)
		return -1;
	else if (a->r > b->r)
		return 1;

	return 0;
}

static int
sort_by_green(const void *color_a, const void *color_b)
{
	struct color *a = *((struct color **) color_a);
	struct color *b = *((struct color **) color_b);

	if (a->g < b->g)
		return -1;
	else if (a->g > b->g)
		return 1;

	return 0;
}

static int
sort_by_blue(const void *color_a, const void *color_b)
{
	struct color *a = *((struct color **) color_a);
	struct color *b = *((struct color **) color_b);

	if (a->b < b->b)
		return -1;
	else if (a->b > b->b)
		return 1;

	return 0;
}

static void
sort_palette(struct array *palette, int by)
{
	int (*compare)(const void *, const void *);

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
	}

	qsort(palette->items, palette->size, sizeof(void *), compare);
}

/* average color of palette */
static struct color *
palette_average(struct array *palette)
{
	struct color *avg;
	size_t i;
	DWORD rsum, gsum, bsum;

	rsum = gsum = bsum = 0;
	avg = xmalloc(sizeof(struct color));

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
median_cut(struct array *palette, int ncuts, struct array *reduced)
{
	struct array *top, *bottom;
	size_t median, i;

	/*
	 * if done add the average color of the bucket
	 * to the reduced palette.
	 */
	if (ncuts == 0) {
		struct color *avg_color;

		avg_color = palette_average(palette);
		array_add(reduced, avg_color);

		return;
	}

	/* sort palette by highest color val of r,g,b */
	sort_palette(palette, max_color(palette));

	/* split into top and bottom part */
	median = palette->size >> 1;
	bottom = array_new();
	top = array_new();

	for (i = 0; i < median; ++i)
		array_add(bottom, palette->items[i]);

	for (i = median; i < palette->size; ++i)
		array_add(top, palette->items[i]);

	/* repeat for the two created buckets */
	median_cut(bottom, ncuts - 1, reduced);
	median_cut(top, ncuts - 1, reduced);

	array_free(bottom);
	array_free(top);
}

static void
print_binary(FILE *outfile, BYTE value)
{
	int bit = 0;

	for (bit = 0; bit < 8; ++bit) {
		int bitmask = (1 << (7 - bit));

		if (value & bitmask)
			fputc('1', outfile);
		else
			fputc('0', outfile);
	}
}

void
write_palette(struct array *palette, char *filename)
{
	FILE *outfile;
	size_t i;

	outfile = fopen(filename, "w");
	if (outfile == NULL)
		xerror("write_palette: can't open file.");

	for (i = 0; i < palette->size; ++i) {
		struct color *color = palette->items[i];
		BYTE ega_r = color->r >> 6;
		BYTE ega_g = color->g >> 6;
		BYTE ega_b = color->b >> 6;

		fprintf(outfile,
		    "#%02x%02x%02x => #%02x%02x%02x => %d %d %d => %02x => ",
		    color->r, color->g, color->b,
		    ega_r * 85, ega_g * 85, ega_b * 85,
		    ega_r, ega_g, ega_b, ega_make_color(color));
		print_binary(outfile, ega_make_color(color));
		fputc('\n', outfile);
	}

	fclose(outfile);
}

