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

struct error {
	int r, g, b, Y;
};

static struct error err[2][MAX_IMAGE_WIDTH];

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

	memset(err, 0, sizeof(err));
	for (row = 0; row < bmp->height - 1; ++row) {
		size_t ofs = row * bmp->width;

		maybe_exit();
		printf("D:%3d%%\r", row * 100 / bmp->height);
		fflush(stdout);
		for (col = 1; col < bmp->width - 1; ++col) {
			struct rgb *color;
			BYTE oldpixel, newpixel;
			int Yerr;

			color = &bmp->palette[bmp->image[ofs + col]];
			oldpixel = clamp(color_to_mono(color) + err[0][col].Y);
			newpixel = (oldpixel * ncolors / 256) * (256 / ncolors);
			bmp->image[ofs + col] = newpixel;

			Yerr = oldpixel - newpixel;
			err[0][col + 1].Y += Yerr * 7 / 16;
			err[1][col - 1].Y += Yerr * 3 / 16;
			err[1][col    ].Y += Yerr * 5 / 16;
			err[1][col + 1].Y += Yerr * 1 / 16;
		}

		memcpy(&err[0][0], &err[1][0], sizeof(err[0]));
		memset(&err[1][0], 0, sizeof(err[1]));
	}
}

/*
 * Dither a bitmap in place
 */
void
dither(struct bitmap *bmp, struct rgb *palette, int ncolors)
{
	int row, col;

	memset(err, 0, sizeof(err));
	for (row = 0; row < bmp->height - 1; ++row) {
		size_t ofs = row * bmp->width;

		maybe_exit();
		printf("D:%3d%%\r", row * 100 / bmp->height);
		fflush(stdout);
		for (col = 1; col < bmp->width - 1; ++col) {
			struct rgb oldpixel, newpixel;
			struct rgb *color;
			int rerr, gerr, berr;
			int palidx;

			color = &bmp->palette[bmp->image[ofs + col]];
			oldpixel.r = clamp(color->r + err[0][col].r);
			oldpixel.g = clamp(color->g + err[0][col].g);
			oldpixel.b = clamp(color->b + err[0][col].b);

			palidx = pick(&oldpixel, palette, ncolors);
			bmp->image[ofs + col] = palidx;

			color = &palette[palidx];
			newpixel.r = color->r;
			newpixel.g = color->g;
			newpixel.b = color->b;

			rerr = oldpixel.r - newpixel.r;
			gerr = oldpixel.g - newpixel.g;
			berr = oldpixel.b - newpixel.b;

			err[0][col + 1].r += rerr * 7 / 16;
			err[0][col + 1].g += gerr * 7 / 16;
			err[0][col + 1].b += berr * 7 / 16;

			err[1][col - 1].r += rerr * 3 / 16;
			err[1][col - 1].g += gerr * 3 / 16;
			err[1][col - 1].b += berr * 3 / 16;

			err[1][col    ].r += rerr * 5 / 16;
			err[1][col    ].g += gerr * 5 / 16;
			err[1][col    ].b += berr * 5 / 16;

			err[1][col + 1].r += rerr * 1 / 16;
			err[1][col + 1].g += gerr * 1 / 16;
			err[1][col + 1].b += berr * 1 / 16;
		}

		memcpy(&err[0][0], &err[1][0], sizeof(err[0]));
		memset(&err[1][0], 0, sizeof(err[1]));
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
		size_t ofs = row * bmp->width;
		BYTE Mrow = row & 7;

		maybe_exit();
		printf("D:%3d%%\r", row * 100 / bmp->height);
		fflush(stdout);

		for (col = 0; col < bmp->width; ++col) {
			int palidx = bmp->image[ofs + col];
			struct rgb *color = &bmp->palette[palidx];
			struct rgb newcolor;
			BYTE Mcol = col & 7;

			newcolor.r = color->r > (4 * M[Mrow][Mcol]) ? 255 : 0;
			newcolor.g = color->g > (4 * M[Mrow][Mcol]) ? 255 : 0;
			newcolor.b = color->b > (4 * M[Mrow][Mcol]) ? 255 : 0;

			palidx = pick(&newcolor, palette, ncolors);
			bmp->image[ofs + col] = palidx;
		}
	}
}
