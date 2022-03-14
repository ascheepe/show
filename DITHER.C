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

#include "bitmap.h"
#include "color.h"
#include "dither.h"
#include "ega.h"

#define INDEX(x, y) ((y) * bmp->width + (x))

void
convert_to_grayscale(struct bitmap *bmp)
{
	int row, col;

	for (row = 0; row < bmp->height; ++row) {
		if (show_progress)
			printf("G:%03d\r", row);

		for (col = 0; col < bmp->width; ++col) {
			struct color *c =
			    &bmp->palette[bmp->image[INDEX(col, row)]];
			BYTE Y = (3 * c->r / 10) + (59 * c->g / 100) +
			    (11 * c->b / 100);

			bmp->image[INDEX(col, row)] = Y;
		}
	}
}

void
dither(struct bitmap *bmp, int ncolors)
{
	int row, col;

	for (row = 0; row < bmp->height; ++row) {
		if (show_progress)
			printf("D:%03d\r", row);

		for (col = 0; col < bmp->width; ++col) {
			BYTE Y = bmp->image[INDEX(col, row)];
			BYTE newY = ((Y * ncolors) / 256) * (256 / ncolors);
			BYTE error = Y - newY;

			bmp->image[INDEX(col, row)] = newY;

			if (col + 1 < bmp->width)
				bmp->image[INDEX(col + 1, row)] +=
				    (error * 7) >> 4;

			if (col - 1 > 0 && row + 1 < bmp->height)
				bmp->image[INDEX(col - 1, row + 1)] +=
				    (error * 3) >> 4;

			if (row + 1 < bmp->height)
				bmp->image[INDEX(col, row + 1)] +=
				    (error * 5) >> 4;

			if (col + 1 < bmp->width && row + 1 < bmp->height)
				bmp->image[INDEX(col + 1, row + 1)] +=
				    error >> 4;
		}
	}
}

int
pick(const struct color *c, const struct color *pal, int ncolors)
{
	DWORD maxdist = -1;
	DWORD dist;
	int i, match;

	for (i = 0; i < ncolors; ++i) {
		const struct color *pc = &pal[i];
		int rdiff, gdiff, bdiff;

		rdiff = c->r - pc->r;
		gdiff = c->g - pc->g;
		bdiff = c->b - pc->b;

		dist = SQR(rdiff) + SQR(gdiff) + SQR(bdiff);
		if (dist < maxdist) {
			maxdist = dist;
			match = i;
		}
	}

	return match;
}

BYTE
color_to_luma(const struct color *c)
{
	return (3 * c->r / 10) + (59 * c->g / 100) + (11 * c->b / 100);
}

void
egadither(struct bitmap *bmp)
{
	struct color egapal[] = {
		{ 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0xAA },
		{ 0x00, 0xAA, 0x00 },
		{ 0x00, 0xAA, 0xAA },
		{ 0xAA, 0x00, 0x00 },
		{ 0xAA, 0x00, 0xAA },
		{ 0xAA, 0x55, 0x00 },
		{ 0xAA, 0xAA, 0xAA },
		{ 0x55, 0x55, 0x55 },
		{ 0x55, 0x55, 0xFF },
		{ 0x55, 0xFF, 0x55 },
		{ 0x55, 0xFF, 0xFF },
		{ 0xFF, 0x55, 0x55 },
		{ 0xFF, 0x55, 0xFF },
		{ 0xFF, 0xFF, 0x55 },
		{ 0xFF, 0xFF, 0xFF }
	};
	struct color error[2][320] = { 0 };
	int row, col, rowoff, coloff;

	coloff = 320 / 2 - bmp->width / 2;
	rowoff = 200 / 2 - bmp->height / 2;

	for (row = 0; row < bmp->height; ++row) {
		if (show_progress)
			printf("D:%03d\r", row);

		for (col = 0; col < bmp->width; ++col) {
			struct color *oldpixel, *newpixel;
			BYTE rerr, gerr, berr;
			int palidx;

			oldpixel = &bmp->palette[bmp->image[INDEX(col, row)]];
			oldpixel->r += error[0][col].r;
			oldpixel->g += error[0][col].g;
			oldpixel->b += error[0][col].b;
			palidx = pick(oldpixel, egapal, 16);
			ega_plot(col + coloff, row + rowoff, palidx);

			newpixel = &egapal[palidx];
			rerr = oldpixel->r - newpixel->r;
			gerr = oldpixel->g - newpixel->g;
			berr = oldpixel->b - newpixel->b;

			if (col + 1 < bmp->width) {
				error[0][col + 1].r += (rerr * 7) >> 4;
				error[0][col + 1].g += (gerr * 7) >> 4;
				error[0][col + 1].b += (berr * 7) >> 4;
			}

			if (col - 1 > 0 && row + 1 < bmp->height) {
				error[1][col - 1].r += (rerr * 3) >> 4;
				error[1][col - 1].g += (gerr * 3) >> 4;
				error[1][col - 1].b += (berr * 3) >> 4;
			}

			if (row + 1 < bmp->height) {
				error[1][col].r += (rerr * 5) >> 4;
				error[1][col].g += (gerr * 5) >> 4;
				error[1][col].b += (berr * 5) >> 4;
			}

			if (col + 1 < bmp->width && row + 1 < bmp->height) {
				error[1][col + 1].r += rerr >> 4;
				error[1][col + 1].g += gerr >> 4;
				error[1][col + 1].b += berr >> 4;
			}
		}

		memcpy(&error[0][0], &error[1][0], sizeof(struct color) * 320);
		memset(&error[1][0], 0, sizeof(struct color) * 320);
	}
}
