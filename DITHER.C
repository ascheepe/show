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

#include <stdlib.h>
#include <string.h>

#include "bitmap.h"
#include "color.h"
#include "dither.h"
#include "system.h"

#define INDEX(x, y) ((y) * bmp->width + (x))

struct error {
	int r, g, b, Y;
};

static struct error error[2][MAX_IMAGE_WIDTH];

/*
 * Clamp a value between 0 and 255, inclusive.
 */
static BYTE
clamp(int value)
{
	if (value > 255)
		return 255;

	if (value < 0)
		return 0;

	return value;
}

#define SQR(n) ((DWORD)((n)*(n)))

/*
 * Finds the closest color to 'color' in palette
 * 'palette', returning the index of it.
 */
int
pick(const struct rgb *color, const struct rgb *palette, int ncolors)
{
	DWORD dist, maxdist = -1;
	int i, match;

	for (i = 0; i < ncolors; ++i) {
		WORD rdiff = abs(color->r - palette[i].r);
		WORD gdiff = abs(color->g - palette[i].g);
		WORD bdiff = abs(color->b - palette[i].b);

		dist = SQR(rdiff) * 3 + SQR(gdiff) * 4 + SQR(bdiff) * 2;

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
	int row, col;

	memset(error, 0, sizeof(error));
	for (row = 0; row < bmp->height - 1; ++row) {
		maybe_exit();
		printf("D:%3d%%\r", row * 100 / bmp->height);
		fflush(stdout);
		for (col = 1; col < bmp->width - 1; ++col) {
			struct rgb *color;
			BYTE old_pixel, new_pixel;
			int Y_error;

			color = &bmp->palette[bmp->image[INDEX(col, row)]];
			old_pixel = clamp(color_to_mono(color) + error[0][col].Y);
			new_pixel = (old_pixel * ncolors / 256) * (256 / ncolors);
			bmp->image[INDEX(col, row)] = new_pixel;

			Y_error = old_pixel - new_pixel;
			error[0][col + 1].Y += Y_error * 7 / 16;
			error[1][col - 1].Y += Y_error * 3 / 16;
			error[1][col    ].Y += Y_error * 5 / 16;
			error[1][col + 1].Y += Y_error * 1 / 16;
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
	int row, col;

	memset(error, 0, sizeof(error));
	for (row = 0; row < bmp->height - 1; ++row) {
		maybe_exit();
		printf("D:%3d%%\r", row * 100 / bmp->height);
		fflush(stdout);
		for (col = 1; col < bmp->width - 1; ++col) {
			struct rgb old_pixel, new_pixel;
			struct rgb *color;
			int red_error, green_error, blue_error;
			int palidx;

			color = &bmp->palette[bmp->image[INDEX(col, row)]];
			old_pixel.r = clamp(color->r + error[0][col].r);
			old_pixel.g = clamp(color->g + error[0][col].g);
			old_pixel.b = clamp(color->b + error[0][col].b);

			palidx = pick(&old_pixel, palette, ncolors);
			bmp->image[INDEX(col, row)] = palidx;

			color = &palette[palidx];
			new_pixel.r = color->r;
			new_pixel.g = color->g;
			new_pixel.b = color->b;

			red_error   = old_pixel.r - new_pixel.r;
			green_error = old_pixel.g - new_pixel.g;
			blue_error  = old_pixel.b - new_pixel.b;

			error[0][col + 1].r += red_error   * 7 / 16;
			error[0][col + 1].g += green_error * 7 / 16;
			error[0][col + 1].b += blue_error  * 7 / 16;

			error[1][col - 1].r += red_error   * 3 / 16;
			error[1][col - 1].g += green_error * 3 / 16;
			error[1][col - 1].b += blue_error  * 3 / 16;

			error[1][col    ].r += red_error   * 5 / 16;
			error[1][col    ].g += green_error * 5 / 16;
			error[1][col    ].b += blue_error  * 5 / 16;

			error[1][col + 1].r   += red_error   * 1 / 16;
			error[1][col + 1].g += green_error * 1 / 16;
			error[1][col + 1].b  += blue_error  * 1 / 16;
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
		{ 12, 44,  4, 36, 14, 46, 6, 38 },
		{ 60, 28, 52, 20, 62, 30, 54, 22 },
		{  3, 35, 11, 43,  1, 33,  9, 41 },
		{ 51, 19, 59, 27, 49, 17, 57, 25 },
		{ 15, 47,  7, 39, 13, 45,  5, 37 },
		{ 63, 31, 55, 23, 61, 29, 53, 21 }
	};
	int row, col;

	for (row = 0; row < bmp->height; ++row) {
		BYTE Mrow = row & 7;

		maybe_exit();
		printf("D:%3d%%\r", row * 100 / bmp->height);
		fflush(stdout);

		for (col = 0; col < bmp->width; ++col) {
			int palidx = bmp->image[INDEX(col, row)];
			struct rgb *color = &bmp->palette[palidx];
			struct rgb new_color;
			BYTE Mcol = col & 7;

			new_color.r = color->r > (4 * M[Mrow][Mcol]) ? 255 : 0;
			new_color.g = color->g > (4 * M[Mrow][Mcol]) ? 255 : 0;
			new_color.b = color->b > (4 * M[Mrow][Mcol]) ? 255 : 0;

			palidx = pick(&new_color, palette, ncolors);
			bmp->image[INDEX(col, row)] = palidx;
		}
	}
}
