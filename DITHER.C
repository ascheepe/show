/*
 * Copyright (c) 2020-2024 Axel Scheepers
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

#include <stdlib.h>
#include <string.h>

#include "bitmap.h"
#include "color.h"
#include "dither.h"
#include "system.h"

struct dither_err {
	int r, g, b, Y;
};

static struct dither_err error[2][MAX_IMAGE_WIDTH];

#define CLAMP(n) ((n) > 255 ? 255 : (n) < 0 ? 0 : (n))
#define SQUARE(n) ((DWORD)((n)*(n)))

/*
 * Finds the closest color to 'color' in palette
 * 'palette', returning the index of it.
 */
static int
find_closest_color(const struct rgb *color, const struct rgb *palette,
    int ncolors)
{
	DWORD dist = -1, maxdist = -1;
	WORD i, match;

	for (i = 0; i < ncolors; ++i) {
		WORD r_dist = abs(color->r - palette[i].r);
		WORD g_dist = abs(color->g - palette[i].g);
		WORD b_dist = abs(color->b - palette[i].b);

		dist = SQUARE(r_dist) + SQUARE(g_dist) + SQUARE(b_dist);

		if (dist < maxdist) {
			maxdist = dist;
			match = i;
		}
	}
	return match;
}


/*
 * Convert bitmap to grayscale with dithering in place.
 */
void
grayscale_dither(struct bitmap *bmp, int ncolors)
{
	WORD row, col;

	memset(error, 0, sizeof(error));
	for (row = 0; row < bmp->height - 1; ++row) {
		WORD offset = row * bmp->width;

		maybe_exit();
		printf("D:%3d%%\r", row * 100 / bmp->height);
		fflush(stdout);
		for (col = 1; col < bmp->width - 1; ++col) {
			struct rgb *color;
			BYTE old, new;
			int Yerr;

			color = &bmp->palette[bmp->image[offset + col]];
			old = CLAMP(color_to_mono(color) + error[0][col].Y);
			new = (old * ncolors / 256) * (256 / ncolors);
			bmp->image[offset + col] = new;

			Yerr = old - new;
			error[0][col + 1].Y += Yerr * 7 / 16;
			error[1][col + 0].Y += Yerr * 5 / 16;
			error[1][col - 1].Y += Yerr * 3 / 16;
			error[1][col + 1].Y += Yerr * 1 / 16;
		}
		memcpy(&error[0][0], &error[1][0], sizeof(error[0]));
		memset(&error[1][0], 0, sizeof(error[1]));
	}
}


/*
 * Dither a bitmap in place
 */
void
dither(struct bitmap *bmp, struct rgb *palette, int ncolors)
{
	WORD row, col;

	memset(error, 0, sizeof(error));
	for (row = 0; row < bmp->height - 1; ++row) {
		WORD offset = row * bmp->width;

		maybe_exit();
		printf("D:%3d%%\r", row * 100 / bmp->height);
		fflush(stdout);
		for (col = 1; col < bmp->width - 1; ++col) {
			struct rgb old, new, *color;
			int r_err, g_err, b_err;
			BYTE i;

			color = &bmp->palette[bmp->image[offset + col]];
			old.r = CLAMP(color->r + error[0][col].r);
			old.g = CLAMP(color->g + error[0][col].g);
			old.b = CLAMP(color->b + error[0][col].b);

			i = find_closest_color(&old, palette, ncolors);
			bmp->image[offset + col] = i;

			color = &palette[i];
			new.r = color->r;
			new.g = color->g;
			new.b = color->b;

			r_err = old.r - new.r;
			g_err = old.g - new.g;
			b_err = old.b - new.b;

			error[0][col + 1].r += r_err * 7 / 16;
			error[0][col + 1].g += g_err * 7 / 16;
			error[0][col + 1].b += b_err * 7 / 16;
			error[1][col + 0].r += r_err * 5 / 16;
			error[1][col + 0].g += g_err * 5 / 16;
			error[1][col + 0].b += b_err * 5 / 16;
			error[1][col - 1].r += r_err * 3 / 16;
			error[1][col - 1].g += g_err * 3 / 16;
			error[1][col - 1].b += b_err * 3 / 16;
			error[1][col + 1].r += r_err * 1 / 16;
			error[1][col + 1].g += g_err * 1 / 16;
			error[1][col + 1].b += b_err * 1 / 16;
		}
		memcpy(&error[0][0], &error[1][0], sizeof(error[0]));
		memset(&error[1][0], 0, sizeof(error[1]));
	}
}

void
ordered_dither(struct bitmap *bmp, struct rgb *palette, int ncolors)
{
	BYTE M[8][8] = {
		{  0, 32,  8, 40,  2, 34, 10, 42 },
		{ 48, 16, 56, 24, 50, 18, 58, 26 },
		{ 12, 44,  4, 36, 14, 46,  6, 38 },
		{ 60, 28, 52, 20, 62, 30, 54, 22 },
		{  3, 35, 11, 43,  1, 33,  9, 41 },
		{ 51, 19, 59, 27, 49, 17, 57, 25 },
		{ 15, 47,  7, 39, 13, 45,  5, 37 },
		{ 63, 31, 55, 23, 61, 29, 53, 21 }
	};
	WORD row, col;

	for (row = 0; row < bmp->height; ++row) {
		WORD offset = row * bmp->width;
		BYTE Mrow = row & 7;

		maybe_exit();
		printf("D:%3d%%\r", row * 100 / bmp->height);
		fflush(stdout);
		for (col = 0; col < bmp->width; ++col) {
			BYTE i = bmp->image[offset + col];
			struct rgb *color = &bmp->palette[i];
			struct rgb new;
			BYTE Mcol = col & 7;

			new.r = color->r > (4 * M[Mrow][Mcol]) ? 255 : 0;
			new.g = color->g > (4 * M[Mrow][Mcol]) ? 255 : 0;
			new.b = color->b > (4 * M[Mrow][Mcol]) ? 255 : 0;
			i = find_closest_color(&new, palette, ncolors);
			bmp->image[offset + col] = i;
		}
	}
}
