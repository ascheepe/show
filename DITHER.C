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
	int red;
	int green;
	int blue;
	int Y;
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
int pick(const struct color *color, const struct color *palette, int ncolors)
{
	DWORD dist, maxdist = -1;
	int i, match;

	for (i = 0; i < ncolors; ++i) {
		const struct color *palette_color = &palette[i];
		WORD red_diff   = abs(color->red   - palette_color->red);
		WORD green_diff = abs(color->green - palette_color->green);
		WORD blue_diff  = abs(color->blue  - palette_color->blue);

		dist = SQR(red_diff) * 3 + SQR(green_diff) * 4 +
		       SQR(blue_diff) * 2;

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
			struct color *color_ptr;
			BYTE old_pixel, new_pixel;
			int Y_error;

			color_ptr = &bmp->palette[bmp->image[INDEX(col, row)]];
			old_pixel = clamp(color_to_mono(color_ptr) +
			    error[0][col].Y);
			new_pixel =
			    (old_pixel * ncolors / 256) * (256 / ncolors);
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
dither(struct bitmap *bmp, struct color *palette, int ncolors)
{
	int row, col;

	memset(error, 0, sizeof(error));
	for (row = 0; row < bmp->height - 1; ++row) {
		maybe_exit();
		printf("D:%3d%%\r", row * 100 / bmp->height);
		fflush(stdout);
		for (col = 1; col < bmp->width - 1; ++col) {
			struct color old_pixel, new_pixel;
			struct color *color_ptr;
			int red_error, green_error, blue_error;
			int palidx;

			color_ptr = &bmp->palette[bmp->image[INDEX(col, row)]];
			old_pixel.red =
			    clamp(color_ptr->red + error[0][col].red);
			old_pixel.green =
			    clamp(color_ptr->green + error[0][col].green);
			old_pixel.blue =
			    clamp(color_ptr->blue + error[0][col].blue);

			palidx = pick(&old_pixel, palette, ncolors);
			bmp->image[INDEX(col, row)] = palidx;

			color_ptr = &palette[palidx];
			new_pixel.red   = color_ptr->red;
			new_pixel.green = color_ptr->green;
			new_pixel.blue  = color_ptr->blue;

			red_error   = old_pixel.red   - new_pixel.red;
			green_error = old_pixel.green - new_pixel.green;
			blue_error  = old_pixel.blue  - new_pixel.blue;

			error[0][col + 1].red   += red_error   * 7 / 16;
			error[0][col + 1].green += green_error * 7 / 16;
			error[0][col + 1].blue  += blue_error  * 7 / 16;

			error[1][col - 1].red   += red_error   * 3 / 16;
			error[1][col - 1].green += green_error * 3 / 16;
			error[1][col - 1].blue  += blue_error  * 3 / 16;

			error[1][col    ].red   += red_error   * 5 / 16;
			error[1][col    ].green += green_error * 5 / 16;
			error[1][col    ].blue  += blue_error  * 5 / 16;

			error[1][col + 1].red   += red_error   * 1 / 16;
			error[1][col + 1].green += green_error * 1 / 16;
			error[1][col + 1].blue  += blue_error  * 1 / 16;
		}

		memcpy(&error[0][0], &error[1][0], sizeof(error[0]));
		memset(&error[1][0], 0, sizeof(error[1]));
	}
}

void
ordered_dither(struct bitmap *bmp, struct color *palette, int ncolors)
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
			struct color *color_ptr = &bmp->palette[palidx];
			struct color new_color;
			BYTE Mcol = col & 7;

			new_color.red = color_ptr->red > (4 * M[Mrow][Mcol])
			    ? 255 : 0;
			new_color.green =
			    color_ptr->green > (4 * M[Mrow][Mcol])
			    ? 255 : 0;
			new_color.blue = color_ptr->blue > (4 * M[Mrow][Mcol])
			    ? 255 : 0;

			palidx = pick(&new_color, palette, ncolors);
			bmp->image[INDEX(col, row)] = palidx;
		}
	}
}
