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

#include "bitmap.h"
#include "dither.h"

#define INDEX(x, y) ((y) * bmp->width + (x))

void convert_to_grayscale(struct bitmap *bmp)
{
    int row;
    int col;

    for (row = 0; row < bmp->height; ++row) {
        if (g_show_progress) {
            printf("G:%03d\r", row);
        }

        for (col = 0; col < bmp->width; ++col) {
            BYTE offset = bmp->image[INDEX(col, row)];
            BYTE red = bmp->palette[offset + 0];
            BYTE green = bmp->palette[offset + 1];
            BYTE blue = bmp->palette[offset + 2];
            BYTE luma = (3 * red / 10) + (59 * green / 100) +
                        (11 * blue / 100);

            bmp->image[INDEX(col, row)] = luma;
        }
    }
}

void dither(struct bitmap *bmp, int n_colors)
{
    int row;
    int col;

    for (row = 0; row < bmp->height; ++row) {
        if (g_show_progress) {
            printf("D:%03d\r", row);
        }

        for (col = 0; col < bmp->width; ++col) {
            BYTE luma = bmp->image[INDEX(col, row)];
            BYTE new_luma = ((luma * n_colors) / 256) * (256 / n_colors);
            BYTE error = luma - new_luma;

            bmp->image[INDEX(col, row)] = new_luma;

            if (col + 1 < bmp->width) {
                bmp->image[INDEX(col + 1, row)] += (error * 7) >> 4;
            }

            if (col - 1 > 0 && row + 1 < bmp->height) {
                bmp->image[INDEX(col - 1, row + 1)] += (error * 3) >> 4;
            }

            if (row + 1 < bmp->height) {
                bmp->image[INDEX(col, row + 1)] += (error * 5) >> 4;
            }

            if (col + 1 < bmp->width && row + 1 < bmp->height) {
                bmp->image[INDEX(col + 1, row + 1)] += error >> 4;
            }
        }
    }
}

