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
		WORD w[3] = { 3, 4, 2 };
		WORD dist;

		diff.r = abs((int)color->r - palette[i].r);
		diff.g = abs((int)color->g - palette[i].g);
		diff.b = abs((int)color->b - palette[i].b);

		/* Adjust color weights */
		if ((color->r + palette[i].r) / 2 < 128) {
			w[0] = 2;
			w[2] = 3;
		}

		dist = w[0] * diff.r + w[1] * diff.g + w[2] * diff.b;
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
	static int error[2][MAX_IMAGE_WIDTH + 2];
	BYTE i0 = (row + 0) & 1;
	BYTE i1 = (row + 1) & 1;
	WORD col;

	if (row == 0)
		memset(error, 0, sizeof(error));
	else
		memset(error[i1], 0, sizeof(error[i1]));

	for (col = 0; col < MAX_IMAGE_WIDTH; ++col) {
		struct rgb *color;
		BYTE i, oldcolor, newcolor;
		WORD idx;
		int diff;

		/* adjust for col-1 and col+1 */
		idx = col + 1;

		color = &image_palette[image_row[col]];
		oldcolor = CLAMP(color_to_mono(color) + error[i0][idx]);
		i = oldcolor * (ncolors - 1) / 255;
		plot(col, row, palette[i]);
		newcolor = i * 255 / (ncolors - 1);

		diff = oldcolor - newcolor;
		error[i0][idx + 1] += diff * 7 / 16;
		error[i1][idx + 0] += diff * 5 / 16;
		error[i1][idx - 1] += diff * 3 / 16;
		error[i1][idx + 1] += diff * 1 / 16;
	}
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

	static struct dither_error error[2][MAX_IMAGE_WIDTH + 2];
	BYTE i0 = (row + 0) & 1;
	BYTE i1 = (row + 1) & 1;
	WORD col;

	if (row == 0)
		memset(error, 0, sizeof(error));
	else
		memset(error[i1], 0, sizeof(error[i1]));

	for (col = 0; col < MAX_IMAGE_WIDTH; ++col) {
		struct rgb oldcolor, newcolor, *color;
		struct dither_error diff;
		WORD idx;
		BYTE i;

		/* adjust for col-1 and col+1 */
		idx = col + 1;

		color = &image_palette[image_row[col]];
		oldcolor.r = CLAMP(color->r + error[i0][idx].r);
		oldcolor.g = CLAMP(color->g + error[i0][idx].g);
		oldcolor.b = CLAMP(color->b + error[i0][idx].b);

		i = pick_color(&oldcolor, palette, ncolors);
		plot(col, row, i);
		color = &palette[i];
		newcolor.r = color->r;
		newcolor.g = color->g;
		newcolor.b = color->b;

		diff.r = oldcolor.r - newcolor.r;
		diff.g = oldcolor.g - newcolor.g;
		diff.b = oldcolor.b - newcolor.b;

		error[i0][idx + 1].r += diff.r * 7 / 16;
		error[i0][idx + 1].g += diff.g * 7 / 16;
		error[i0][idx + 1].b += diff.b * 7 / 16;
		error[i1][idx + 0].r += diff.r * 5 / 16;
		error[i1][idx + 0].g += diff.g * 5 / 16;
		error[i1][idx + 0].b += diff.b * 5 / 16;
		error[i1][idx - 1].r += diff.r * 3 / 16;
		error[i1][idx - 1].g += diff.g * 3 / 16;
		error[i1][idx - 1].b += diff.b * 3 / 16;
		error[i1][idx + 1].r += diff.r * 1 / 16;
		error[i1][idx + 1].g += diff.g * 1 / 16;
		error[i1][idx + 1].b += diff.b * 1 / 16;
	}
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