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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system.h"
#include "bitmap.h"
#include "color.h"
#include "system.h"

#define FILEHEADERSIZE 14

/*
 * Read a .BMP file into a bitmap struct.
 * Also checks if it's a valid file
 * (320x200, 256c + alpha, uncompressed).
 */
struct bitmap *
bitmap_read(char *filename)
{
	struct bitmap *bmp;
	FILE *f;
	DWORD width, row;
	int i, to_skip;

	f = fopen(filename, "rb");
	if (f == NULL)
		die("bitmap_read:");

	bmp = xmalloc(sizeof(struct bitmap));

	bmp->file_type = read_word(f);
	if (bmp->file_type != 0x4d42)
		die("bitmap_read: not a bitmap file.");

	bmp->file_size = read_dword(f);
	bmp->reserved = read_dword(f);
	bmp->pixel_offset = read_dword(f);
	bmp->header_size = read_dword(f);
	bmp->width = read_dword(f);
	bmp->height = read_dword(f);

	if (bmp->width == 0 || bmp->height == 0 ||
	    bmp->width > MAX_IMAGE_WIDTH || bmp->height > MAX_IMAGE_HEIGHT)
		die("bitmap_read: image must be 320x200 or less.");

	bmp->planes = read_word(f);
	bmp->bpp = read_word(f);
	if (bmp->bpp > 8)
		die("bitmap_read: unsupported bit depth (%d).", bmp->bpp);

	bmp->compression = read_dword(f);
	if (bmp->compression != 0)
		die("bitmap_read: compression is not supported.");

	bmp->image_size = read_dword(f);
	bmp->x_ppm = read_dword(f);
	bmp->y_ppm = read_dword(f);
	bmp->ncolors = read_dword(f);
	bmp->palette = xmalloc(sizeof(*bmp->palette) * bmp->ncolors);

	bmp->ncolors_important = read_dword(f);

	/* palette data is bgr(a), located after all the headers */
	fseek(f, bmp->header_size + FILEHEADERSIZE, SEEK_SET);
	for (i = 0; i < bmp->ncolors; ++i) {
		bmp->palette[i].b = read_byte(f);
		bmp->palette[i].g = read_byte(f);
		bmp->palette[i].r = read_byte(f);
		read_byte(f);	/* read away alpha value */
	}

	/* fill remaining palette with black */
	for (; i < 256; ++i) {
		bmp->palette[i].r = 0;
		bmp->palette[i].g = 0;
		bmp->palette[i].b = 0;
	}

	bmp->image = xmalloc(bmp->width * bmp->height);

	/* pad width to multiple of 4 */
	width = ((bmp->width + 3) / 4) * 4;
	to_skip = width - bmp->width;

	/* read the image data */
	fseek(f, bmp->pixel_offset, SEEK_SET);
	row = bmp->height;
	while (row-- > 0) {
		BYTE *row_ptr = bmp->image + row * bmp->width;

		maybe_exit();
		printf("L:%3d%%\r", 100 - row * 100 / bmp->height);
		fflush(stdout);

		if (fread(row_ptr, bmp->width, 1, f) != 1)
			die("bitmap_read: input error.");

		if (to_skip > 0)
			fseek(f, to_skip, SEEK_CUR);
	}

	fclose(f);
	return bmp;
}

void
bitmap_free(struct bitmap *bmp)
{
	free(bmp->image);
	free(bmp->palette);
	free(bmp);
}

struct bitmap *
bitmap_copy(struct bitmap *bmp)
{
	struct bitmap *copy;

	copy = xmalloc(sizeof(*copy));
	memcpy(copy, bmp, sizeof(*copy));

	copy->image = xmalloc(bmp->height * bmp->width);
	memcpy(copy->image, bmp->image, bmp->height * bmp->width);

	copy->palette = xmalloc(sizeof(struct rgb) * bmp->ncolors);
	memcpy(copy->palette, bmp->palette,
	    sizeof(struct rgb) * bmp->ncolors);

	return copy;
}
