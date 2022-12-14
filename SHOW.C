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
#include "quantize.h"
#include "mda.h"
#include "cga.h"
#include "ega.h"
#include "vga.h"

static struct color ega_palette[] = {
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
    { 0x00, 0x00, 0xAA },
    { 0x00, 0x00, 0xFF },
    { 0xAA, 0x55, 0x00 },
    { 0xFF, 0xAA, 0x00 },
    { 0xFF, 0x00, 0xFF },
    { 0x00, 0xFF, 0xFF },
};

static BYTE cga_palette[4] = {
    0, 2, 1, 3
};

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

static void show(char *filename)
{
    struct bitmap *bmp;
    int row_offset;
    int col_offset;
    int row;
    int col;

    /* clear screen */
    set_mode(MODE_TEXT);

    bmp = bitmap_read(filename);

    switch (detect_graphics()) {
        case MDA_GRAPHICS:
            col_offset = MDA_WIDTH  / 2 - bmp->width  / 2;
            row_offset = MDA_HEIGHT / 2 - bmp->height / 2;
            grayscale_dither(bmp, 2);

            mda_set_mode(MDA_GRAPHICS_MODE);
            mda_clear_screen();

            for (row = 0; row < bmp->height; ++row) {
                for (col = 0; col < bmp->width; ++col) {
                    BYTE luma = bmp->image[row * bmp->width + col] >> 7;

                    mda_plot(col + col_offset, row + row_offset, luma);
                }
            }
        break;

        case CGA_GRAPHICS:
            col_offset = CGA_WIDTH  / 2 - bmp->width  / 2;
            row_offset = CGA_HEIGHT / 2 - bmp->height / 2;
            grayscale_dither(bmp, 4);

            set_mode(MODE_CGA2);

            for (row = 0; row < bmp->height; ++row) {
                for (col = 0; col < bmp->width; ++col) {
                    BYTE luma = bmp->image[row * bmp->width + col] >> 6;
                    BYTE color;

                    color = cga_palette[luma];
                    cga_plot(col + col_offset, row + row_offset, color);
                }
            }
        break;

        case EGA_GRAPHICS:
            col_offset = EGA_WIDTH  / 2 - bmp->width  / 2;
            row_offset = EGA_HEIGHT / 2 - bmp->height / 2;
            dither(bmp, ega_palette, 16);

            set_mode(MODE_EGAHI);
            ega_set_palette(ega_palette, 16);

            for (row = 0; row < bmp->height - 1; ++row) {
                for (col = 1; col < bmp->width - 1; ++col) {
                    BYTE color;

                    color = bmp->image[row * bmp->width + col];
                    ega_hi_plot(col + col_offset, row + row_offset, color);
                }
            }
        break;

        case VGA_GRAPHICS:
            col_offset = VGA_WIDTH  / 2 - bmp->width  / 2;
            row_offset = VGA_HEIGHT / 2 - bmp->height / 2;

            set_mode(MODE_VGA);
            vga_set_palette(bmp->palette);

            for (row = 0; row < bmp->height; ++row) {
                BYTE *source      = bmp->image + row * bmp->width;
                BYTE *destination = vga_vmem
                                  + VGA_MEM_OFFSET(col_offset,
                                                   row + row_offset);

                memcpy(destination, source, bmp->width);
            }
        break;
    }

    bitmap_free(bmp);

    while (!quit()) {
        /* wait */
    }

    set_mode(MODE_TEXT);
}


int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("usage: show imagefile\n");
        return 1;
    }

    show(argv[1]);

    return 0;
}

