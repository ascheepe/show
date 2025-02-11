#include "pcx.h"
#include "detect.h"
#include "dither.h"
#include "globals.h"
#include "system.h"

#include "mda.h"
#include "cga.h"
#include "cplus.h"
#include "ega.h"
#include "vga.h"

static void
pcx_read_row(FILE *fp)
{
	int xpos = 0;

	maybe_exit();

	while (xpos < image_width) {
		BYTE pixelvalue;

		pixelvalue = read_byte(fp);
		if (pixelvalue >= 192) {
			BYTE count = pixelvalue - 192;

			if (xpos + count > image_width)
				die("pcx_read_row: rle overflow.");

			pixelvalue = read_byte(fp);
			while (count-- > 0)
				image_row[xpos++] = pixelvalue;
		} else {
			image_row[xpos++] = pixelvalue;
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
	DWORD i, row;

	fp = fopen(filename, "rb");
	if (fp == NULL)
		die("pcx_show: can't open file '%s'.", filename);

	if (read_byte(fp) != 0x0a)
		die("pcx_show: not a pcx file.");

	/* version = */ read_byte(fp);
	if (read_byte(fp) != 0x01)
		die("pcx_show: only rle compressed images are supported.");
	if (read_byte(fp) != 0x08)
		die("pcx_show: only 8 bits per plane are supported.");
	if (read_word(fp) != 0x0000 || read_word(fp) != 0x0000)
		die("pcx_show: minimum x/y coordinates should be zero.");
	image_width = read_word(fp) + 1;
	if (image_width > 320)
		die("pcx_show: image is too wide.");
	image_height = read_word(fp) + 1;
	if (image_height > 200)
		die("pcx_show: image is too tall.");
	if (fseek(fp, 53, SEEK_CUR) != 0)
		die("pcx_show: short read.");
	if (read_byte(fp) != 0x01)
		die("pcx_show: number of planes should be one.");
	if (fseek(fp, -769, SEEK_END) != 0 || read_byte(fp) != 0x0c)
		die("pcx_show: 256 color palette missing.");

	for (i = 0; i < 256; ++i) {
		image_palette[i].r = read_byte(fp);
		image_palette[i].g = read_byte(fp);
		image_palette[i].b = read_byte(fp);
	}

	switch (graphics_mode) {
	case MDA_GRAPHICS:
		x_offset = MDA_WIDTH / 2 - image_width / 2;
		y_offset = MDA_HEIGHT / 2 - image_height / 2;
		mda_clear_screen();
		break;
	case CGA_GRAPHICS:
		x_offset = CGA_WIDTH / 2 - image_width / 2;
		y_offset = CGA_HEIGHT / 2 - image_height / 2;
		cga_clear_screen();
		break;
	case CPLUS_GRAPHICS:
		x_offset = CPLUS_WIDTH / 2 - image_width / 2;
		y_offset = CPLUS_HEIGHT / 2 - image_height / 2;
		cplus_clear_screen();
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
		pcx_read_row(fp);
		show_row(row);
	}
	fclose(fp);

	while (maybe_exit() == 0)
		;
}
