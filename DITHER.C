#include <stdlib.h>
#include <string.h>

#include "detect.h"
#include "globals.h"
#include "system.h"

#include "mda.h"
#include "cga.h"
#include "cplus.h"
#include "tga.h"
#include "ega.h"
#include "vga.h"

#define CLAMP(n) ((n) > 255 ? 255 : (n) < 0 ? 0 : (n))

/*
 * Finds the closest color to 'color' in palette
 * 'palette', returning the index of it.
 */
static int
pick_color(const struct rgb *color, const struct rgb *palette, int ncolors)
{
	WORD i, match, maxdist;

	maxdist = -1;
	for (i = 0; i < ncolors; ++i) {
		struct rgb diff;
		WORD dist;

		diff.r = abs((int)color->r - palette[i].r);
		diff.g = abs((int)color->g - palette[i].g);
		diff.b = abs((int)color->b - palette[i].b);

		dist = diff.r + diff.g + diff.b;
		if (dist < maxdist) {
			maxdist = dist;
			match = i;
		}
	}

	return match;
}

static BYTE
color_to_mono(struct rgb *color)
{
	return
		color->r * 30 / 100 +
		color->g * 59 / 100 +
		color->b * 11 / 100;
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

	for (col = 1; col < MAX_IMAGE_WIDTH - 1; ++col) {
		struct rgb *color;
		BYTE i, oldcolor, newcolor;
		int err;

		color = &image_palette[image_row[col]];
		oldcolor = CLAMP(color_to_mono(color) + p0[col]);
		i = oldcolor * ncolors / 256;
		plot(col, row, palette[i]);
		newcolor = i * 256 / ncolors;

		err = oldcolor - newcolor;
		p0[col + 1] += err * 7 / 16;
		p1[col + 0] += err * 5 / 16;
		p1[col - 1] += err * 3 / 16;
		p1[col + 1] += err * 1 / 16;
	}

	memset(p0, 0, MAX_IMAGE_WIDTH);
	ptmp = p0;
	p0 = p1;
	p1 = ptmp;
}

/*
 * Dither and plot a row in color.
 */
static void
color_dither(int row, struct rgb *palette, int ncolors)
{
	struct dither_error {
		int r, g, b;
	};

	static struct dither_error error[2][MAX_IMAGE_WIDTH];
	static struct dither_error *p0 = &error[0][0];
	static struct dither_error *p1 = &error[1][0];
	struct dither_error *ptmp;
	WORD col;

	if (row == 0)
		memset(error, 0, sizeof(error));

	for (col = 1; col < MAX_IMAGE_WIDTH - 1; ++col) {
		struct rgb oldcolor, newcolor, *color;
		struct dither_error err;
		BYTE i;

		color = &image_palette[image_row[col]];
		oldcolor.r = CLAMP(color->r + p0[col].r);
		oldcolor.g = CLAMP(color->g + p0[col].g);
		oldcolor.b = CLAMP(color->b + p0[col].b);

		i = pick_color(&oldcolor, palette, ncolors);
		plot(col, row, i);
		color = &palette[i];
		newcolor.r = color->r;
		newcolor.g = color->g;
		newcolor.b = color->b;

		err.r = oldcolor.r - newcolor.r;
		err.g = oldcolor.g - newcolor.g;
		err.b = oldcolor.b - newcolor.b;

		p0[col + 1].r += err.r * 7 / 16;
		p0[col + 1].g += err.g * 7 / 16;
		p0[col + 1].b += err.b * 7 / 16;
		p1[col + 0].r += err.r * 5 / 16;
		p1[col + 0].g += err.g * 5 / 16;
		p1[col + 0].b += err.b * 5 / 16;
		p1[col - 1].r += err.r * 3 / 16;
		p1[col - 1].g += err.g * 3 / 16;
		p1[col - 1].b += err.b * 3 / 16;
		p1[col + 1].r += err.r * 1 / 16;
		p1[col + 1].g += err.g * 1 / 16;
		p1[col + 1].b += err.b * 1 / 16;
	}

	memset(p0, 0, sizeof(struct dither_error) * MAX_IMAGE_WIDTH);
	ptmp = p0;
	p0 = p1;
	p1 = ptmp;
}

void
show_row(int row)
{
	switch (graphics_mode) {
	case MDA_GRAPHICS:
		grayscale_dither(row, mda_palette, 2);
		break;
	case CGA_GRAPHICS:
		grayscale_dither(row, cga_palette, 4);
		break;
	case CPLUS_GRAPHICS:
	case TGA_GRAPHICS:
	case EGA_GRAPHICS:
		color_dither(row, std_palette, 16);
		break;
	case VGA_GRAPHICS:
		vga_plot_row(row, image_row);
		break;
	}
}
