#include "bitmap.h"
#include "color.h"
#include "detect.h"
#include "dither.h"
#include "globals.h"
#include "system.h"

#include "mda.h"
#include "cga.h"
#include "ega.h"
#include "vga.h"

#define FILEHEADERSIZE 14

/*
 * Read a .BMP file into a bitmap struct.
 * Also checks if it's a valid file
 * (320x200, 256c + alpha, uncompressed).
 */
void
bitmap_show(char *filename)
{
	FILE *fp;
	DWORD width, row;
	DWORD header_size, pixel_offset;
	DWORD ncolors;
	int i, skip;

	fp = fopen(filename, "rb");
	if (fp == NULL)
		die("bitmap_read:");

	if (read_word(fp) != 0x4d42)
		die("bitmap_read: not a bitmap file.");

	/* file_size = */ read_dword(fp);
	/* reserved  = */ read_dword(fp);
	pixel_offset = read_dword(fp);
	header_size = read_dword(fp);
	image_width = read_dword(fp);
	image_height = read_dword(fp);

	if (image_width == 0 || image_height == 0 ||
	    image_width > MAX_IMAGE_WIDTH || image_height > MAX_IMAGE_HEIGHT)
		die("bitmap_read: image must be 320x200 or less.");

	/* planes = */ read_word(fp);
	if (read_word(fp) > 8)
		die("bitmap_read: unsupported bit depth.");

	if (read_dword(fp) != 0)
		die("bitmap_read: compression is not supported.");

	/* image_size = */ read_dword(fp);
	/* x_ppm      = */ read_dword(fp);
	/* y_ppm      = */ read_dword(fp);

	ncolors = read_dword(fp);
	/* ncolors_important = */ read_dword(fp);

	/* palette data is bgr(a), located after all the headers */
	fseek(fp, header_size + FILEHEADERSIZE, SEEK_SET);
	for (i = 0; i < ncolors; ++i) {
		image_palette[i].b = read_byte(fp);
		image_palette[i].g = read_byte(fp);
		image_palette[i].r = read_byte(fp);

		/* read away alpha value */
		read_byte(fp);
	}

	/* fill remaining palette with black */
	for (; i < 256; ++i) {
		image_palette[i].r = 0;
		image_palette[i].g = 0;
		image_palette[i].b = 0;
	}

	/* pad width to multiple of 4 */
	width = ((image_width + 3) / 4) * 4;
	skip = width - image_width;

	/* read, dither and show the image data */
	fseek(fp, pixel_offset, SEEK_SET);
	fseek(fp, image_height * width - width, SEEK_CUR);

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

	case EGA_GRAPHICS:
		x_offset = EGA_WIDTH / 2 - image_width / 2;
		y_offset = EGA_HEIGHT / 2 - image_height / 2;
		ega_clear_screen();
		ega_set_palette(ega_palette, 16);
		break;

	case VGA_GRAPHICS:
		x_offset = VGA_WIDTH / 2 - image_width / 2;
		y_offset = VGA_HEIGHT / 2 - image_height / 2;
		vga_clear_screen();
		vga_set_palette(image_palette);
		break;
	}

	dither_init();
	for (row = 0; row < image_height; ++row) {
		maybe_exit();

		if (fread(image_row, image_width, 1, fp) != 1)
			die("bitmap_read: input error.");

		show(row);

		if (skip > 0)
			fseek(fp, skip, SEEK_CUR);

		fseek(fp, -width * 2, SEEK_CUR);
	}

	fclose(fp);
}
