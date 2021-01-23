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

#include <stdio.h>
#include <stdlib.h>

#include "system.h"
#include "bitmap.h"

struct bitmap *
bitmap_read(char *filename)
{
	struct bitmap *bmp;
	FILE *bmpfile;
	DWORD padded_width;
	DWORD row;
	DWORD pal_offset;
	int i;

	bmpfile = fopen(filename, "rb");
	if (bmpfile == NULL)
		xerror("can't open file");

	bmp = malloc(sizeof(struct bitmap));
	if (bmp == NULL)
		xerror("can't allocate image struct");

	bmp->filetype = read_word(bmpfile);
	if (bmp->filetype != 0x4d42)
		xerror("not a bitmap file");

	bmp->filesize = read_dword(bmpfile);
	bmp->reserved = read_dword(bmpfile);
	bmp->pixeloffset = read_dword(bmpfile);
	bmp->headersize = read_dword(bmpfile);
	pal_offset = 14 + bmp->headersize;
	bmp->width = read_dword(bmpfile);
	bmp->height = read_dword(bmpfile);

	if ((bmp->width == 0) || (bmp->height == 0) ||
	    (bmp->width > 320) || (bmp->height > 200))
		xerror("image must be 320x200 or less");

	bmp->planes = read_word(bmpfile);
	bmp->bpp = read_word(bmpfile);
	if (bmp->bpp > 8)
		xerror("unsupported bit depth");

	bmp->compression = read_dword(bmpfile);
	if (bmp->compression != 0)
		xerror("compression is not supported");

	bmp->imagesize = read_dword(bmpfile);
	bmp->xppm = read_dword(bmpfile);
	bmp->yppm = read_dword(bmpfile);
	bmp->ncolors = read_dword(bmpfile);
	bmp->ncolors_important = read_dword(bmpfile);

	/* palette data is bgr(a) */
	fseek(bmpfile, pal_offset, SEEK_SET);
	for (i = 0; i < bmp->ncolors; ++i) {
		int offset = i * 3;

		bmp->palette[offset + 2] = read_byte(bmpfile);
		bmp->palette[offset + 1] = read_byte(bmpfile);
		bmp->palette[offset + 0] = read_byte(bmpfile);
		read_byte(bmpfile);	/* read away alpha value */
	}

	/* fill remaining palette with black */
	for (; i < 256; ++i) {
		int offset = i * 3;

		bmp->palette[offset + 0] = 0;
		bmp->palette[offset + 1] = 0;
		bmp->palette[offset + 2] = 0;
	}

	bmp->image = malloc(bmp->width * bmp->height);
	if (bmp->image == NULL)
		xerror("can't allocate memory for image");

	/* pad width to multiple of 4 with formula (x + 4-1) & (-4) */
	padded_width = (bmp->width + 4 - 1) & -4;
	for (row = 0; row < bmp->height; ++row) {
		BYTE *rowptr = bmp->image + row * bmp->width;

		if (g_show_progress)
			printf("L:%03d\r", row);

		/* bitmap stores data beginning at the last line */
		fseek(bmpfile, bmp->pixeloffset + (bmp->height - row - 1) *
		    padded_width, SEEK_SET);
		fread(rowptr, bmp->width, 1, bmpfile);
	}

	fclose(bmpfile);

	return bmp;
}

void
bitmap_free(struct bitmap *bmp)
{
	free(bmp->image);
	free(bmp);
}

