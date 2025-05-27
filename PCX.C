#include <string.h>

#include "pcx.h"
#include "detect.h"
#include "dither.h"
#include "globals.h"
#include "system.h"

#include "mda.h"
#include "cga.h"
#include "cplus.h"
#include "tga.h"
#include "ega.h"
#include "vga.h"

static void
read_row(FILE *fp)
{
	u16 xpos = 0;

	maybe_exit();

	memset(image_row, 0, sizeof(image_row));

	while (xpos < image_width) {
		u8 val;

		val = read8(fp);
		if (val >= 192) {
			u8 count = val - 192;

			if (xpos + count > image_width)
				die("read_row: rle overflow.");

			val = read8(fp);
			while (count-- > 0)
				image_row[x_offset + xpos++] = val;
		} else {
			image_row[x_offset + xpos++] = val;
		}
	}
}

/*
 * Read, dither and display a .PCX file.
 */
void
pcx_show(char *filename)
{
	FILE *fp;
	u32 i, row;

	fp = fopen(filename, "rb");
	if (fp == NULL)
		die("pcx_show: can't open file '%s'.", filename);

	if (read8(fp) != 0x0a)
		die("pcx_show: not a pcx file.");

	/* version = */ read8(fp);
	if (read8(fp) != 0x01)
		die("pcx_show: only rle compressed images are supported.");
	if (read8(fp) != 0x08)
		die("pcx_show: only 8 bits per plane are supported.");
	if (read16(fp) != 0x0000 || read16(fp) != 0x0000)
		die("pcx_show: minimum x/y coordinates should be zero.");
	image_width = read16(fp) + 1;
	if (image_width > 320)
		die("pcx_show: image is too wide.");
	image_height = read16(fp) + 1;
	if (image_height > 200)
		die("pcx_show: image is too tall.");
	if (fseek(fp, 53, SEEK_CUR) != 0)
		die("pcx_show: short read.");
	if (read8(fp) != 0x01)
		die("pcx_show: number of planes should be one.");
	if (fseek(fp, -769, SEEK_END) != 0 || read8(fp) != 0x0c)
		die("pcx_show: 256 color palette missing.");

	for (i = 0; i < 256; ++i) {
		image_palette[i].r = read8(fp);
		image_palette[i].g = read8(fp);
		image_palette[i].b = read8(fp);
	}

	switch (graphics_mode) {
	case MDA_GRAPHICS:
		x_offset = MDA_WIDTH / 2 - image_width / 2;
		y_offset = MDA_HEIGHT / 2 - image_height / 2;
		break;
	case CGA_GRAPHICS:
		x_offset = CGA_WIDTH / 2 - image_width / 2;
		y_offset = CGA_HEIGHT / 2 - image_height / 2;
		break;
	case CPLUS_GRAPHICS:
		x_offset = CPLUS_WIDTH / 2 - image_width / 2;
		y_offset = CPLUS_HEIGHT / 2 - image_height / 2;
		break;
	case TGA_GRAPHICS:
		x_offset = TGA_WIDTH / 2 - image_width / 2;
		y_offset = TGA_HEIGHT / 2 - image_height / 2;
		break;
	case EGA_GRAPHICS:
		x_offset = EGA_WIDTH / 2 - image_width / 2;
		y_offset = EGA_HEIGHT / 2 - image_height / 2;
		ega_clear_screen();
		break;
	case VGA_GRAPHICS:
		x_offset = VGA_WIDTH / 2 - image_width / 2;
		y_offset = VGA_HEIGHT / 2 - image_height / 2;
		vga_clear_screen();
		vga_set_palette(image_palette);
		break;
	}

	/* read, dither and show the image data */
	fseek(fp, 128, SEEK_SET);
	for (row = 0; row < image_height; ++row) {
		read_row(fp);
		show_row(row);
	}
	fclose(fp);

	while (maybe_exit() == 0)
		;
}
