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

struct error {
	int r, g, b, Y;
};

static struct error error[2][MAX_IMAGE_WIDTH];

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
			BYTE old_color, new_color;
			int Yerror;

			color = &bmp->palette[bmp->image[offset + col]];
			old_color =
			    CLAMP(color_to_mono(color) + error[0][col].Y);
			new_color =
			    (old_color * ncolors / 256) * (256 / ncolors);
			bmp->image[offset + col] = new_color;

			Yerror = old_color - new_color;
			error[0][col + 1].Y += Yerror * 7 / 16;
			error[1][col + 0].Y += Yerror * 5 / 16;
			error[1][col - 1].Y += Yerror * 3 / 16;
			error[1][col + 1].Y += Yerror * 1 / 16;
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
			struct rgb old_color, new_color, *color;
			int r_error, g_error, b_error;
			BYTE i;

			color = &bmp->palette[bmp->image[offset + col]];
			old_color.r = CLAMP(color->r + error[0][col].r);
			old_color.g = CLAMP(color->g + error[0][col].g);
			old_color.b = CLAMP(color->b + error[0][col].b);

			i = find_closest_color(&old_color, palette, ncolors);
			bmp->image[offset + col] = i;

			color = &palette[i];
			new_color.r = color->r;
			new_color.g = color->g;
			new_color.b = color->b;

			r_error = old_color.r - new_color.r;
			g_error = old_color.g - new_color.g;
			b_error = old_color.b - new_color.b;

			error[0][col + 1].r += r_error * 7 / 16;
			error[0][col + 1].g += g_error * 7 / 16;
			error[0][col + 1].b += b_error * 7 / 16;
			error[1][col + 0].r += r_error * 5 / 16;
			error[1][col + 0].g += g_error * 5 / 16;
			error[1][col + 0].b += b_error * 5 / 16;
			error[1][col - 1].r += r_error * 3 / 16;
			error[1][col - 1].g += g_error * 3 / 16;
			error[1][col - 1].b += b_error * 3 / 16;
			error[1][col + 1].r += r_error * 1 / 16;
			error[1][col + 1].g += g_error * 1 / 16;
			error[1][col + 1].b += b_error * 1 / 16;
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
			struct rgb new_color;
			BYTE Mcol = col & 7;

			new_color.r = color->r > (4 * M[Mrow][Mcol]) ? 255 : 0;
			new_color.g = color->g > (4 * M[Mrow][Mcol]) ? 255 : 0;
			new_color.b = color->b > (4 * M[Mrow][Mcol]) ? 255 : 0;
			i = find_closest_color(&new_color, palette, ncolors);
			bmp->image[offset + col] = i;
		}
	}
}
