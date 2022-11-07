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
#include "color.h"
#include "dither.h"
#include "ega.h"

#define INDEX(x, y) ((y) * bmp->width + (x))

void convert_to_grayscale(struct bitmap *bmp)
{
    int row;
    int col;

    for (row = 0; row < bmp->height; ++row) {
        if (show_progress) {
            printf("G:%03d\r", row);
        }

        for (col = 0; col < bmp->width; ++col) {
            struct color *color = &bmp->palette[bmp->image[INDEX(col, row)]];

            bmp->image[INDEX(col, row)] = color_to_luma(color);
        }
    }
}

void dither(struct bitmap *bmp, int ncolors)
{
    int row;
    int col;

    for (row = 0; row < bmp->height; ++row) {
        if (show_progress) {
            printf("D:%03d\r", row);
        }

        for (col = 0; col < bmp->width; ++col) {
            BYTE Y = bmp->image[INDEX(col, row)];
            BYTE newY = ((Y * ncolors) / 256) * (256 / ncolors);
            BYTE error = Y - newY;

            bmp->image[INDEX(col, row)] = newY;

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

static int pick(const struct color *color,
                const struct color *palette,
                int ncolors)
{
    DWORD max_distance = -1;
    DWORD distance;
    int match;
    int i;

    for (i = 0; i < ncolors; ++i) {
        const struct color *palette_color = &palette[i];
        int red_diff = color->red - palette_color->red;
        int green_diff = color->green - palette_color->green;
        int blue_diff = color->blue - palette_color->blue;

        distance = SQR(red_diff) + SQR(green_diff) + SQR(blue_diff);
        if (distance < max_distance) {
            max_distance = distance;
            match = i;
        }
    }

    return match;
}

static BYTE add_colors(int a, int b)
{
    int result = a + b;

    if (result > 255) {
        return 255;
    }

    if (result < 0) {
        return 0;
    }

    return result;
}

void egadither(struct bitmap *bmp)
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
    struct color quant_error[2][320] = { 0 };
    int row_offset;
    int column_offset;
    int row;
    int col;

    column_offset = 320 / 2 - bmp->width / 2;
    row_offset = 200 / 2 - bmp->height / 2;

    for (row = 0; row < bmp->height; ++row) {
        for (col = 0; col < bmp->width; ++col) {
            struct color *pixel;
            struct color *egapixel;
            int palette_index;
            int red_error;
            int green_error;
            int blue_error;

            pixel = &bmp->palette[bmp->image[INDEX(col, row)]];
            pixel->red = add_colors(pixel->red, quant_error[0][col].red);
            pixel->green = add_colors(pixel->green, quant_error[0][col].green);
            pixel->blue = add_colors(pixel->blue, quant_error[0][col].blue);
            palette_index = pick(pixel, palette, 16);
            ega_plot(col + column_offset, row + row_offset, palette_index);

            egapixel = &palette[palette_index];
            red_error = pixel->red - egapixel->red;
            green_error = pixel->green - egapixel->green;
            blue_error = pixel->blue - egapixel->blue;

            if (col + 1 < bmp->width) {
                quant_error[0][col + 1].red =
                    add_colors(quant_error[0][col + 1].red,
                               (red_error * 7) >> 4);
                quant_error[0][col + 1].green =
                    add_colors(quant_error[0][col + 1].green,
                               (green_error * 7) >> 4);
                quant_error[0][col + 1].blue =
                    add_colors(quant_error[0][col + 1].blue,
                               (blue_error * 7) >> 4);
            }

            if (col - 1 > 0 && row + 1 < bmp->height) {
                quant_error[1][col - 1].red =
                    add_colors(quant_error[1][col - 1].red,
                               (red_error * 3) >> 4);
                quant_error[1][col - 1].green =
                    add_colors(quant_error[1][col - 1].green,
                               (green_error * 3) >> 4);
                quant_error[1][col - 1].blue =
                    add_colors(quant_error[1][col - 1].blue,
                               (blue_error * 3) >> 4);
            }

            if (row + 1 < bmp->height) {
                quant_error[1][col].red =
                    add_colors(quant_error[1][col].red,
                               (red_error * 5) >> 4);
                quant_error[1][col].green =
                    add_colors(quant_error[1][col].green,
                               (green_error * 5) >> 4);
                quant_error[1][col].blue =
                    add_colors(quant_error[1][col].blue,
                               (blue_error * 5) >> 4);
            }

            if (col + 1 < bmp->width && row + 1 < bmp->height) {
                quant_error[1][col + 1].red =
                    add_colors(quant_error[1][col + 1].red,
                               red_error >> 4);
                quant_error[1][col + 1].green =
                    add_colors(quant_error[1][col + 1].green,
                               green_error >> 4);
                quant_error[1][col + 1].blue =
                    add_colors(quant_error[1][col + 1].blue,
                               blue_error >> 4);
            }
        }

        memcpy(&quant_error[0][0], &quant_error[1][0],
               sizeof(struct color) * 320);
        memset(&quant_error[1][0], 0, sizeof(struct color) * 320);
    }
}

