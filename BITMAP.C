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

#define FILEHEADERSIZE 14

/*
 * Read a .BMP file into a bitmap struct.
 * Also checks if it's a valid file
 * (320x200, 256c + alpha, uncompressed).
 */
struct bitmap *bitmap_read(char *filename)
{
    struct bitmap *bmp = NULL;
    FILE *bmp_file = NULL;
    DWORD width;
    DWORD row;
    int to_skip;
    int i;

    bmp_file = fopen(filename, "rb");
    if (bmp_file == NULL) {
        xerror("can't open file");
    }

    bmp = malloc(sizeof(struct bitmap));
    if (bmp == NULL) {
        xerror("can't allocate image struct");
    }

    bmp->file_type = read_word(bmp_file);
    if (bmp->file_type != 0x4d42) {
        xerror("not a bitmap file");
    }

    bmp->file_size    = read_dword(bmp_file);
    bmp->reserved     = read_dword(bmp_file);
    bmp->pixel_offset = read_dword(bmp_file);
    bmp->header_size  = read_dword(bmp_file);
    bmp->width        = read_dword(bmp_file);
    bmp->height       = read_dword(bmp_file);

    if (bmp->width == 0 || bmp->height == 0
        || bmp->width > MAX_IMAGE_WIDTH || bmp->height > MAX_IMAGE_HEIGHT) {
        xerror("image must be 320x200 or less");
    }

    bmp->planes = read_word(bmp_file);
    if ((bmp->bpp = read_word(bmp_file)) > 8) {
        xerror("unsupported bit depth");
    }

    bmp->compression = read_dword(bmp_file);
    if (bmp->compression != 0) {
        xerror("compression is not supported");
    }

    bmp->image_size = read_dword(bmp_file);
    bmp->x_ppm      = read_dword(bmp_file);
    bmp->y_ppm      = read_dword(bmp_file);
    bmp->ncolors    = read_dword(bmp_file);
    bmp->palette    = malloc(sizeof(*bmp->palette) * bmp->ncolors);
    if (bmp->palette == NULL) {
        xerror("can't allocate memory for image palette");
    }
    bmp->ncolors_important = read_dword(bmp_file);

    /* palette data is bgr(a), located after all the headers */
    fseek(bmp_file, bmp->header_size + FILEHEADERSIZE, SEEK_SET);
    for (i = 0; i < bmp->ncolors; ++i) {
        bmp->palette[i].blue  = read_byte(bmp_file);
        bmp->palette[i].green = read_byte(bmp_file);
        bmp->palette[i].red   = read_byte(bmp_file);
        read_byte(bmp_file); /* read away alpha value */
    }

    /* fill remaining palette with black */
    for (; i < 256; ++i) {
        bmp->palette[i].red   = 0;
        bmp->palette[i].green = 0;
        bmp->palette[i].blue  = 0;
    }

    bmp->image = malloc(bmp->width * bmp->height);
    if (bmp->image == NULL) {
        xerror("can't allocate memory for image");
    }

    /* pad width to multiple of 4 with formula (x + 4-1) & (-4) */
    width = (bmp->width + 4 - 1) & -4;
    to_skip = width - bmp->width;

    /* read the image data */
    fseek(bmp_file, bmp->pixel_offset, SEEK_SET);
    for (row = bmp->height; row > 0; --row) {
        BYTE *row_ptr = bmp->image + (row - 1) * bmp->width;

        printf("Loading %3d%%\r", row * 100 / bmp->height);
        fflush(stdout);

        if (fread(row_ptr, bmp->width, 1, bmp_file) != 1) {
            xerror("error reading file.");
        }

        if (to_skip > 0) {
            fseek(bmp_file, to_skip, SEEK_CUR);
        }
    }

    fclose(bmp_file);
    return bmp;
}

void bitmap_free(struct bitmap *bmp)
{
    free(bmp->image);
    free(bmp->palette);
    free(bmp);
}

struct bitmap *bitmap_copy(struct bitmap *bmp)
{
    struct bitmap *copy;

    copy = xmalloc(sizeof(*copy));
    memcpy(copy, bmp, sizeof(*copy));
    copy->image = xmalloc(bmp->width * bmp->height);
    memcpy(copy->image, bmp->image, bmp->width * bmp->height);
    copy->palette = xmalloc(sizeof(struct color) * bmp->ncolors);
    memcpy(copy->palette, bmp->palette, sizeof(struct color) * bmp->ncolors);

    return copy;
}


