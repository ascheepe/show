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

#include <string.h>

#include "bitmap.h"
#include "color.h"
#include "dither.h"

#define INDEX(x, y) ((y) * bmp->width + (x))

struct error {
	int red;
	int green;
	int blue;
	int luma;
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

/*
 * Convert bitmap to grayscale with dithering in place.
 */
void
grayscale_dither(struct bitmap *bmp, int ncolors)
{
	int row, col;

	memset(error, 0, sizeof(error));
	for (row = 0; row < bmp->height - 1; ++row) {
		printf("D:%3d%%\r", row * 100 / bmp->height);
		fflush(stdout);
		for (col = 1; col < bmp->width - 1; ++col) {
			struct color *color_ptr;
			int luma_error;
			BYTE old_px;
			BYTE new_px;

			color_ptr = &bmp->palette[bmp->image[INDEX(col, row)]];
			old_px = clamp(color_to_mono(color_ptr)
			    + error[0][col].luma);
			new_px =
			    (old_px * ncolors / 256) * (256 / ncolors);
			bmp->image[INDEX(col, row)] = new_px;

			luma_error = old_px - new_px;
			error[0][col + 1].luma += luma_error * 7 / 16;
			error[1][col - 1].luma += luma_error * 3 / 16;
			error[1][col].luma += luma_error * 5 / 16;
			error[1][col + 1].luma += luma_error * 1 / 16;
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
		printf("D:%3d%%\r", row * 100 / bmp->height);
		fflush(stdout);
		for (col = 1; col < bmp->width - 1; ++col) {
			struct color old_px, new_px;
			struct color *color_ptr;
			int red_error, green_error, blue_error;
			int palidx;

			color_ptr = &bmp->palette[bmp->image[INDEX(col, row)]];
			old_px.red =
			    clamp(color_ptr->red + error[0][col].red);
			old_px.green =
			    clamp(color_ptr->green + error[0][col].green);
			old_px.blue =
			    clamp(color_ptr->blue + error[0][col].blue);

			palidx = find_closest_color(&old_px, palette,
			    ncolors);
			bmp->image[INDEX(col, row)] = palidx;

			color_ptr = &palette[palidx];
			new_px.red = color_ptr->red;
			new_px.green = color_ptr->green;
			new_px.blue = color_ptr->blue;

			red_error = old_px.red - new_px.red;
			green_error = old_px.green - new_px.green;
			blue_error = old_px.blue - new_px.blue;

			error[0][col + 1].red += red_error * 7 / 16;
			error[0][col + 1].green += green_error * 7 / 16;
			error[0][col + 1].blue += blue_error * 7 / 16;

			error[1][col - 1].red += red_error * 3 / 16;
			error[1][col - 1].green += green_error * 3 / 16;
			error[1][col - 1].blue += blue_error * 3 / 16;

			error[1][col].red += red_error * 5 / 16;
			error[1][col].green += green_error * 5 / 16;
			error[1][col].blue += blue_error * 5 / 16;

			error[1][col + 1].red += red_error * 1 / 16;
			error[1][col + 1].green += green_error * 1 / 16;
			error[1][col + 1].blue += blue_error * 1 / 16;
		}

		memcpy(&error[0][0], &error[1][0], sizeof(error[0]));
		memset(&error[1][0], 0, sizeof(error[1]));
	}
}

void
ordered_dither(struct bitmap *bmp, struct color *palette, int ncolors)
{
	BYTE M[8][8] = {
		{ 0, 32, 8, 40, 2, 34, 10, 42 },
		{ 48, 16, 56, 24, 50, 18, 58, 26 },
		{ 12, 44, 4, 36, 14, 46, 6, 38 },
		{ 60, 28, 52, 20, 62, 30, 54, 22 },
		{ 3, 35, 11, 43, 1, 33, 9, 41 },
		{ 51, 19, 59, 27, 49, 17, 57, 25 },
		{ 15, 47, 7, 39, 13, 45, 5, 37 },
		{ 63, 31, 55, 23, 61, 29, 53, 21 }
	};
	int row, col;

	for (row = 0; row < bmp->height; ++row) {
		BYTE Mrow = row & 7;

		printf("Dithering %3d%%\r", row * 100 / bmp->height);
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

			palidx =
			    find_closest_color(&new_color, palette, ncolors);
			bmp->image[INDEX(col, row)] = palidx;
		}
	}
}
