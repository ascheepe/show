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

#ifndef BITMAP_H
#define BITMAP_H

#include "system.h"

/*
 * .BMP file structure
 */
struct bitmap {
	WORD filetype;
	DWORD filesize;
	DWORD reserved;
	DWORD pixeloffset;
	DWORD headersize;
	DWORD width;
	DWORD height;
	WORD planes;
	WORD bpp;
	DWORD compression;
	DWORD imagesize;
	DWORD xppm;
	DWORD yppm;
	DWORD ncolors;
	DWORD ncolors_important;

	BYTE palette[256 * 3];
	BYTE *image;
};

struct bitmap *bitmap_read(char *);
void bitmap_free(struct bitmap *);

#endif

