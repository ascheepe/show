#include <stdlib.h>
#include <string.h>

#include "color.h"
#include "detect.h"
#include "globals.h"
#include "system.h"

#include "mda.h"
#include "cga.h"
#include "ega.h"
#include "vga.h"

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


/*
 * Dither and plot a row in grayscale.
 */
static void
grayscale_dither(int row, BYTE *palette, int ncolors)
{
	static BYTE error[2][MAX_IMAGE_WIDTH];
	static BYTE *p0 = &error[0][0];
	static BYTE *p1 = &error[1][0];
	BYTE *ptmp;
	WORD col;

	if (row == 0)
		memset(error, 0, sizeof(error));

	for (col = 1; col < image_width - 1; ++col) {
		struct rgb *color;
		BYTE oldcolor, newcolor, palidx;
		int err;

		color = &image_palette[image_row[col]];
		oldcolor = CLAMP(color_to_mono(color) + p0[col]);
		palidx = oldcolor * ncolors / 256;
		newcolor = palidx * (256 / ncolors);

		err = oldcolor - newcolor;
		p0[col + 1] += err * 7 / 16;
		p1[col + 0] += err * 5 / 16;
		p1[col - 1] += err * 3 / 16;
		p1[col + 1] += err * 1 / 16;

		plot(col + x_offset, row + y_offset, palette[palidx]);
	}

	memset(p0, 0, MAX_IMAGE_WIDTH);
	ptmp = p0;
	p0 = p1;
	p1 = ptmp;
}

struct dither_error {
	int r;
	int g;
	int b;
};

/*
 * Dither and plot a row in color.
 */
static void
color_dither(int row, struct rgb *palette, int ncolors)
{
	static struct dither_error error[2][MAX_IMAGE_WIDTH];
	static struct dither_error *p0 = &error[0][0];
	static struct dither_error *p1 = &error[1][0];
	struct dither_error *ptmp;
	WORD col;

	if (row == 0)
		memset(error, 0, sizeof(error));

	for (col = 1; col < image_width - 1; ++col) {
		struct rgb oldcolor, newcolor, *color;
		int err[3];
		BYTE i;

		color = &image_palette[image_row[col]];
		oldcolor.r = CLAMP(color->r + p0[col].r);
		oldcolor.g = CLAMP(color->g + p0[col].g);
		oldcolor.b = CLAMP(color->b + p0[col].b);

		i = pick_color(&oldcolor, palette, ncolors);
		color = &palette[i];
		newcolor.r = color->r;
		newcolor.g = color->g;
		newcolor.b = color->b;

		err[0] = oldcolor.r - newcolor.r;
		err[1] = oldcolor.g - newcolor.g;
		err[2] = oldcolor.b - newcolor.b;

		p0[col + 1].r += err[0] * 7 / 16;
		p0[col + 1].g += err[1] * 7 / 16;
		p0[col + 1].b += err[2] * 7 / 16;
		p1[col + 0].r += err[0] * 5 / 16;
		p1[col + 0].g += err[1] * 5 / 16;
		p1[col + 0].b += err[2] * 5 / 16;
		p1[col - 1].r += err[0] * 3 / 16;
		p1[col - 1].g += err[1] * 3 / 16;
		p1[col - 1].b += err[2] * 3 / 16;
		p1[col + 1].r += err[0] * 1 / 16;
		p1[col + 1].g += err[1] * 1 / 16;
		p1[col + 1].b += err[2] * 1 / 16;

		plot(col + x_offset, row + y_offset, i);
	}

	memset(p0, 0, sizeof(struct dither_error) * MAX_IMAGE_WIDTH);
	ptmp = p0;
	p0 = p1;
	p1 = ptmp;
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