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
#include <dir.h>
#include <dos.h>

#include "system.h"
#include "vector.h"
#include "color.h"
#include "bitmap.h"
#include "detect.h"
#include "dither.h"
#include "mda.h"
#include "cga.h"
#include "ega.h"
#include "vga.h"

static void (*show)(char *);

static void mda_show(char *filename)
{
    struct bitmap *bmp;
    int row_offset, col_offset;
    int row, col;

    bmp = bitmap_read(filename);
    row_offset = 174 - (bmp->height >> 1);
    col_offset = 360 - (bmp->width >> 1);
    grayscale_dither(bmp, 2);

    mda_set_graphics_mode(1);
    mda_clear_screen();

    for (row = 0; row < bmp->height; ++row) {
        for (col = 0; col < bmp->width; ++col) {
            BYTE Y = bmp->image[row * bmp->width + col] >> 7;

            mda_plot(col + col_offset, row + row_offset, Y);
        }
    }

    bitmap_free(bmp);
}

static void cga_show(char *filename)
{
    struct bitmap *bmp;
    int row_offset, col_offset;
    int row, col;

    bmp = bitmap_read(filename);
    row_offset = 100 - (bmp->height >> 1);
    col_offset = 160 - (bmp->width >> 1);
    grayscale_dither(bmp, 4);

    set_mode(MODE_CGA2);
    cga_clear_screen();

    for (row = 0; row < bmp->height; ++row) {
        for (col = 0; col < bmp->width; ++col) {
            BYTE pal[4] = { 0, 2, 1, 3 };
            BYTE Y = bmp->image[row * bmp->width + col] >> 6;

            cga_plot(col + col_offset, row + row_offset, pal[Y]);
        }
    }

    bitmap_free(bmp);
}

static void ega_show(char *filename)
{
    struct color palette[] = {
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0xAA },
        { 0x00, 0xAA, 0x00 },
        { 0x00, 0xAA, 0xAA },
        { 0xAA, 0x00, 0x00 },
        { 0xAA, 0x00, 0xAA },
        { 0xAA, 0x55, 0x00 },
        { 0xAA, 0xAA, 0xAA },
        { 0x55, 0x55, 0x55 },
        { 0x55, 0x55, 0xFF },
        { 0x55, 0xFF, 0x55 },
        { 0x55, 0xFF, 0xFF },
        { 0xFF, 0x55, 0x55 },
        { 0xFF, 0x55, 0xFF },
        { 0xFF, 0xFF, 0x55 },
        { 0xFF, 0xFF, 0xFF }
    };
    struct bitmap *bmp;
    int row_offset, col_offset;
    int row, col;

    bmp = bitmap_read(filename);
    row_offset = 100 - (bmp->height >> 1);
    col_offset = 160 - (bmp->width >> 1);
    dither(bmp, palette, 16);

    set_mode(MODE_EGA);
    ega_clear_screen();

    for (row = 0; row < bmp->height - 1; ++row) {
        for (col = 1; col < bmp->width - 1; ++col) {
            BYTE color;

            color = bmp->image[row * bmp->width + col];
            ega_plot(col + col_offset, row + row_offset, color);
        }
    }

    bitmap_free(bmp);
}

static struct vector *reduce_palette(struct bitmap *bmp)
{
    struct vector *palette;
    struct vector *reduced_palette;
    BYTE raw_palette[256*3];
    int i;

    for (i = 0; i < bmp->ncolors; ++i) {
        int offset = i * 3;

        raw_palette[offset + 0] = bmp->palette[i].red;
        raw_palette[offset + 1] = bmp->palette[i].green;
        raw_palette[offset + 2] = bmp->palette[i].blue;
    }

    palette = palette_to_vector(raw_palette, bmp->ncolors);
    reduced_palette = vector_new();
    median_cut(palette, 4, reduced_palette);
    vector_foreach(palette, free);
    vector_free(palette);

    return reduced_palette;
}

static void ega_hi_show(char *filename)
{
    struct bitmap *bmp;
    struct vector *reduced_palette;
    struct color palette[16];
    int row_offset, col_offset;
    int row, col;
    int i;

    bmp = bitmap_read(filename);
    row_offset = 175 - (bmp->height >> 1);
    col_offset = 320 - (bmp->width >> 1);

    reduced_palette = reduce_palette(bmp);

    for (i = 0; i < 16; ++i) {
        struct color *color = reduced_palette->items[i];

        palette[i].red   = color->red;
        palette[i].green = color->green;
        palette[i].blue  = color->blue;
    }
    vector_foreach(reduced_palette, free);
    vector_free(reduced_palette);
    dither(bmp, palette, 16);

    set_mode(MODE_EGAHI);
    ega_set_palette(reduced_palette);

    for (row = 0; row < bmp->height - 1; ++row) {
        for (col = 1; col < bmp->width - 1; ++col) {
            BYTE color;

            color = bmp->image[row * bmp->width + col];
            ega_hi_plot(col + col_offset, row + row_offset, color);
        }
    }

    bitmap_free(bmp);
}

static void vga_show(char *filename)
{
    struct bitmap *bmp;
    int row_offset, col_offset;
    int row;

    bmp = bitmap_read(filename);
    row_offset = 100 - (bmp->height >> 1);
    col_offset = 160 - (bmp->width >> 1);

    set_mode(MODE_VGA);
    vga_clear_screen();
    vga_set_palette(bmp->palette);
    for (row = 0; row < bmp->height; ++row) {
        BYTE *source = bmp->image + row * bmp->width;
        BYTE *destination = vmem + VGA_MEM_OFFSET(col_offset, row + row_offset);

        memcpy(destination, source, bmp->width);
    }

    bitmap_free(bmp);
}

#define KEY_ESC 27
static int next_or_exit(void)
{
    int key_pressed = false;

    if (kbhit()) {
        key_pressed = true;
        switch (getch()) {
            case 'q':
            case 'Q':
            case KEY_ESC:
                set_mode(MODE_TEXT);
                exit(EXIT_SUCCESS);

                /* read away special key */
            case 0:
            case 224:
                getch();
                break;
        }
    }

    return key_pressed;
}

#define DEFAULT_WAIT_MSEC 10000
#define DELAY 100

static int slideshow(int wait_msec)
{
    struct ffblk ffblk;
    int has_images = false;
    int status;

    for (status = findfirst("*.bmp", &ffblk, 0);
         status == 0;
         status = findnext(&ffblk)) {

        int total_delays = wait_msec / DELAY;
        int ndelays = 0;

        if (ffblk.ff_attrib & FA_DIREC) {
            continue;
        }

        has_images = true;
        set_mode(MODE_TEXT);
        show(ffblk.ff_name);
        while (!next_or_exit() && (ndelays < total_delays)) {
            delay(DELAY);
            ++ndelays;
        }

    }

    return has_images;
}

int main(int argc, char *argv[])
{
    int wait_msec = DEFAULT_WAIT_MSEC;

    /* clear screen */
    set_mode(MODE_TEXT);

    switch (detect_graphics()) {
        case MDA_GRAPHICS:
            show = mda_show;
            break;

        case CGA_GRAPHICS:
            show = cga_show;
            break;

        case EGA_GRAPHICS:
            show = ega_show;
            break;

        case VGA_GRAPHICS:
            show = vga_show;
            break;

        default:
            xerror("Error detecting graphics card.");
    }

    /*
     * If we have an argument it's either a file to show
     * or a delay for a slideshow (and the images will be
     * read from the current directory).
     */
    if (argc == 2) {
        if (file_exists(argv[1])) {
            show(argv[1]);
            while (!next_or_exit());
        } else {
            wait_msec = atoi(argv[1]) * 1000;

            if (wait_msec <= 0) {
                wait_msec = DEFAULT_WAIT_MSEC;
            }
        }
    }

    while (slideshow(wait_msec)) {
        /* loop over found images until user quits */
    }

    xerror("No images found.");
}
