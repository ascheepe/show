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
#include <time.h>
#include <conio.h>
#include <dos.h>

#include "system.h"
#include "color.h"
#include "bitmap.h"
#include "detect.h"
#include "dither.h"
#include "mda.h"
#include "cga.h"
#include "ega.h"
#include "vga.h"

static void mda_show(char *filename)
{
    struct bitmap *bmp;
    int row_offset;
    int col_offset;
    int row;
    int col;

    bmp = bitmap_read(filename);
    row_offset = 174 - (bmp->height >> 1);
    col_offset = 360 - (bmp->width >> 1);
    grayscale_dither(bmp, 2);

    mda_set_graphics_mode(1);
    mda_clear_screen();

    for (row = 0; row < bmp->height; ++row) {
        for (col = 0; col < bmp->width; ++col) {
            BYTE luma = bmp->image[row * bmp->width + col] >> 7;

            mda_plot(col + col_offset, row + row_offset, luma);
        }
    }

    bitmap_free(bmp);
}

static void cga_show(char *filename)
{
    struct bitmap *bmp;
    int row_offset;
    int col_offset;
    int row;
    int col;

    bmp = bitmap_read(filename);
    row_offset = 100 - (bmp->height >> 1);
    col_offset = 160 - (bmp->width >> 1);
    grayscale_dither(bmp, 4);

    set_mode(MODE_CGA2);

    for (row = 0; row < bmp->height; ++row) {
        for (col = 0; col < bmp->width; ++col) {
            BYTE pal[4] = { 0, 2, 1, 3 };
            BYTE luma = bmp->image[row * bmp->width + col] >> 6;

            cga_plot(col + col_offset, row + row_offset, pal[luma]);
        }
    }

    bitmap_free(bmp);
}

static void ega_show(char *filename)
{
    struct color palette[] = {
        { 0x00, 0x00, 0x00 },
        { 0xFF, 0xFF, 0xFF },
        { 0x55, 0x00, 0x00 },
        { 0x55, 0xAA, 0xAA },
        { 0xAA, 0x55, 0xAA },
        { 0x55, 0xAA, 0x55 },
        { 0x00, 0x00, 0x55 },
        { 0xAA, 0xAA, 0x00 },
        { 0xAA, 0x55, 0x00 },
        { 0x55, 0x55, 0x00 },
        { 0xAA, 0x55, 0x55 },
        { 0x55, 0x55, 0x55 },
        { 0xAA, 0xAA, 0xAA },
        { 0xAA, 0xFF, 0xAA },
        { 0xAA, 0xAA, 0xFF },
        { 0x00, 0x55, 0x55 }
    };
    struct bitmap *bmp;
    int row_offset;
    int col_offset;
    int row;
    int col;

    bmp = bitmap_read(filename);
    row_offset = 100 - (bmp->height >> 1);
    col_offset = 160 - (bmp->width >> 1);
    dither(bmp, palette, 16);

    set_mode(MODE_EGA);

    for (row = 0; row < bmp->height - 1; ++row) {
        for (col = 1; col < bmp->width - 1; ++col) {
            BYTE color;

            color = bmp->image[row * bmp->width + col];
            ega_plot(col + col_offset, row + row_offset, color);
        }
    }

    bitmap_free(bmp);
}

static void ega_hi_show(char *filename)
{
    struct color palette[] = {
        { 0x00, 0x00, 0x00 },
        { 0x55, 0x55, 0x55 },
        { 0xAA, 0xAA, 0xAA },
        { 0xFF, 0xFF, 0xFF },
        { 0x55, 0x00, 0x00 },
        { 0xAA, 0x00, 0x00 },
        { 0xFF, 0x00, 0x00 },
        { 0x00, 0x55, 0x00 },
        { 0x00, 0xAA, 0x00 },
        { 0x00, 0xFF, 0x00 },
        { 0x00, 0x00, 0x55 },
        { 0x00, 0x00, 0xAA },
        { 0xAA, 0x55, 0x00 },
        { 0xFF, 0xAA, 0x00 },
        { 0x55, 0xFF, 0xFF },
        { 0xFF, 0x55, 0xFF }
    };
    struct bitmap *bmp;
    int row_offset;
    int col_offset;
    int row;
    int col;

    bmp = bitmap_read(filename);
    row_offset = 175 - (bmp->height >> 1);
    col_offset = 320 - (bmp->width >> 1);
    dither(bmp, palette, 16);

    set_mode(MODE_EGAHI);
    ega_set_palette(palette, 16);

    for (row = 0; row < bmp->height - 1; ++row) {
        for (col = 1; col < bmp->width - 1; ++col) {
            BYTE color;

            color = bmp->image[row * bmp->width + col];
            ega_hi_plot(col + col_offset, row + row_offset, color);
        }
    }

    bitmap_free(bmp);
}

#if 0
struct histogram {
    BYTE index;
    DWORD count;
};

static int descending(const void *histogram_a, const void *histogram_b) {
    struct histogram *a = histogram_a;
    struct histogram *b = histogram_b;

    return b->count - a->count;
}

static void ega_hi_show_custom(char *filename)
{
    struct histogram histogram[256];
    struct color palette[16];
    struct bitmap *bmp;
    int row_offset;
    int col_offset;
    int row;
    int col;
    int i;

    bmp = bitmap_read(filename);
    row_offset = 175 - (bmp->height >> 1);
    col_offset = 320 - (bmp->width >> 1);

    /* take the 16 most used colors for palette */
    memset(histogram, 0, sizeof(histogram));
    for (row = 0; row < bmp->height; ++row) {
        for (col = 0; col < bmp->width; ++col) {
            BYTE index = bmp->image[row * bmp->width + col];

            histogram[index].index = index;
            ++histogram[index].count;
        }
    }

    qsort(histogram, 256, sizeof(struct histogram), descending);

    for (i = 0; i < 16; ++i) {
        struct color *color = &bmp->palette[histogram[i].index];

        palette[i].red   = color->red;
        palette[i].green = color->green;
        palette[i].blue  = color->blue;
    }

    dither(bmp, palette, 16);

    set_mode(MODE_EGAHI);
    ega_set_palette(palette, 16);

    for (row = 0; row < bmp->height - 1; ++row) {
        for (col = 1; col < bmp->width - 1; ++col) {
            BYTE color;

            color = bmp->image[row * bmp->width + col];
            ega_hi_plot(col + col_offset, row + row_offset, color);
        }
    }

    bitmap_free(bmp);
}
#endif

static void vga_show(char *filename)
{
    struct bitmap *bmp;
    int row_offset;
    int col_offset;
    int row;

    bmp = bitmap_read(filename);
    row_offset = 100 - (bmp->height >> 1);
    col_offset = 160 - (bmp->width >> 1);

    set_mode(MODE_VGA);
    vga_set_palette(bmp->palette);
    for (row = 0; row < bmp->height; ++row) {
        BYTE *source = bmp->image + row * bmp->width;
        BYTE *destination = vmem + VGA_MEM_OFFSET(col_offset, row + row_offset);

        memcpy(destination, source, bmp->width);
    }

    bitmap_free(bmp);
}

#define KEY_ESC 27
static int quit(void)
{
    if (kbhit()) {
        switch (getch()) {
            case 'q':
            case 'Q':
            case KEY_ESC:
                return 1;

            /* read away special key */
            case 0:
            case 224:
                getch();
                break;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("usage: show imagefile\n");
        return 1;
    }

    if (!file_exists(argv[1])) {
        printf("file does not exist.\n");
        return 1;
    }

    /* clear screen */
    set_mode(MODE_TEXT);

    switch (detect_graphics()) {
        case MDA_GRAPHICS:
            mda_show(argv[1]);
            break;

        case CGA_GRAPHICS:
            cga_show(argv[1]);
            break;

        case EGA_GRAPHICS:
            ega_hi_show(argv[1]);
            break;

        case VGA_GRAPHICS:
            vga_show(argv[1]);
            break;

        default:
            xerror("Error detecting graphics card.");
    }

    while (!quit()) {
        /* wait */
    }

    set_mode(MODE_TEXT);
    return 0;
}

