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
#include "detect.h"
#include "globals.h"
#include "system.h"

#include "mda.h"
#include "cga.h"
#include "ega.h"
#include "vga.h"

struct dither_error {
	int r, g, b, Y;
};

static struct dither_error error[2][MAX_IMAGE_WIDTH];

#define CLAMP(n) ((n) > 255 ? 255 : (n) < 0 ? 0 : (n))
#define SQUARE(n) ((DWORD)((n)*(n)))

/*
 * Finds the closest color to 'color' in palette
 * 'palette', returning the index of it.
 */
static int
pick_color(const struct rgb *color, const struct rgb *palette, int ncolors)
{
	DWORD dist = -1, maxdist = -1;
	WORD i, match;

	for (i = 0; i < ncolors; ++i) {
		WORD r_dist = abs(color->r - palette[i].r);
		WORD g_dist = abs(color->g - palette[i].g);
		WORD b_dist = abs(color->b - palette[i].b);

		dist =
			SQUARE(r_dist) * 3 +
			SQUARE(g_dist) * 6 +
			SQUARE(b_dist) * 1;

		if (dist < maxdist) {
			maxdist = dist;
			match = i;
		}
	}
	return match;
}


void
dither_init(void)
{
	memset(error, 0, sizeof(error));
}

/*
 * Dither and plot a row in grayscale.
 */
void
grayscale_dither(int row, BYTE *palette, int ncolors)
{
	WORD col;

	for (col = 1; col < image_width - 1; ++col) {
		struct rgb *color;
		BYTE old, new;
		int Yerr;

		color = &image_palette[image_row[col]];
		old = CLAMP(color_to_mono(color) + error[0][col].Y);
		new = (old * ncolors / 256) * (256 / ncolors);

		Yerr = old - new;
		error[0][col + 1].Y += Yerr * 7 / 16;
		error[1][col + 0].Y += Yerr * 5 / 16;
		error[1][col - 1].Y += Yerr * 3 / 16;
		error[1][col + 1].Y += Yerr * 1 / 16;

		plot(col + x_offset, row + y_offset,
		    palette[new / (256 / ncolors)]);

	}

	memcpy(&error[0][0], &error[1][0], sizeof(error[0]));
	memset(&error[1][0], 0, sizeof(error[1]));
}


/*
 * Dither and plot a bitmap.
 */
void
color_dither(int row, struct rgb *palette, int ncolors)
{
	WORD col;

	for (col = 1; col < image_width - 1; ++col) {
		struct rgb old, new, *color;
		int r_err, g_err, b_err;
		BYTE i;

		color = &image_palette[image_row[col]];
		old.r = CLAMP(color->r + error[0][col].r);
		old.g = CLAMP(color->g + error[0][col].g);
		old.b = CLAMP(color->b + error[0][col].b);

		i = pick_color(&old, palette, ncolors);
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

		plot(col + x_offset, row + y_offset, i);

	}

	memcpy(&error[0][0], &error[1][0], sizeof(error[0]));
	memset(&error[1][0], 0, sizeof(error[1]));
}

void
show(int row)
{
	switch (graphics_mode) {
	case MDA_GRAPHICS:
		grayscale_dither(row, mda_palette, 2);
		break;

	case CGA_GRAPHICS:
		grayscale_dither(row, cga_palette, 4);
		break;

	case EGA_GRAPHICS:
		color_dither(row, ega_palette, 16);
		break;

	case VGA_GRAPHICS:
		{
			int col;

			for (col = 0; col < image_width; ++col)
				plot(col + x_offset, row + y_offset,
				    image_row[col]);
		}
		break;
	}
}
