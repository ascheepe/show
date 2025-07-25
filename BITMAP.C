#include "bitmap.h"
#include "detect.h"
#include "dither.h"
#include "globals.h"
#include "system.h"

#include "mda.h"
#include "cga.h"
#include "cplus.h"
#include "ega.h"
#include "vga.h"

#define FILEHEADERSIZE 14

/*
 * Read, dither and display a .BMP file.
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
		die("bitmap_read: can't open file '%s'.", filename);

	if (read16(fp) != 0x4d42)
		die("bitmap_read: not a bitmap file.");

	/* file_size = */ read32(fp);
	/* reserved  = */ read32(fp);
	pixel_offset = read32(fp);
	header_size = read32(fp);
	image_width = read32(fp);
	image_height = read32(fp);

	if (image_width == 0 || image_height == 0 ||
	    image_width > MAX_IMAGE_WIDTH || image_height > MAX_IMAGE_HEIGHT)
		die("bitmap_read: image must be 320x200 or less.");

	/* planes = */ read16(fp);
	if (read16(fp) > 8)
		die("bitmap_read: unsupported bit depth.");

	if (read32(fp) != 0)
		die("bitmap_read: compression is not supported.");

	/* image_size = */ read32(fp);
	/* x_ppm      = */ read32(fp);
	/* y_ppm      = */ read32(fp);

	ncolors = read32(fp);
	/* ncolors_important = */ read32(fp);

	/* palette data is bgr(a), located after all the headers */
	fseek(fp, header_size + FILEHEADERSIZE, SEEK_SET);
	for (i = 0; i < ncolors; ++i) {
		image_palette[i].b = read8(fp);
		image_palette[i].g = read8(fp);
		image_palette[i].r = read8(fp);

		/* read away alpha value */
		read8(fp);
	}

	/* fill remaining palette with black */
	for (; i < 256; ++i) {
		image_palette[i].r = 0;
		image_palette[i].g = 0;
		image_palette[i].b = 0;
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

	/* pad width to multiple of 4 */
	width = ((image_width + 3) / 4) * 4;
	skip = width - image_width;

	/* read, dither and show the image data */
	fseek(fp, pixel_offset + image_height * width - width, SEEK_SET);

	for (row = 0; row < image_height; ++row) {
		maybe_exit();

		if (fread(image_row, image_width, 1, fp) != 1)
			die("bitmap_read: input error.");

		show_row(row);

		fseek(fp, -width * 2 + skip, SEEK_CUR);
	}
	fclose(fp);

	while (maybe_exit() == 0)
		;
}
