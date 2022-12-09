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

static struct bitmap *bmp;

static int get_max_range(int row_start, int row_end)
{
    BYTE red_max   = 0;
    BYTE green_max = 0;
    BYTE blue_max  = 0;
    int row;
    int col;

    for (row = row_start; row < row_end; ++row) {
        for (col = 0; col < bmp->width; ++col) {
            struct color *color;

            color = &bmp->palette[bmp->image[row * bmp->width + col]];
            if (color->red > red_max) {
                red_max = color->red;
            }

            if (color->green > green_max) {
                green_max = color->green;
            }

            if (color->blue > blue_max) {
                blue_max = color->blue;
            }

        }
    }

    if (red_max > green_max && red_max > blue_max) {
        return MAX_RANGE_RED;
    }

    if (green_max > red_max && green_max > blue_max) {
        return MAX_RANGE_GREEN;
    }

    return MAX_RANGE_BLUE;
}

static struct color *get_average_color(int row_start, int row_end)
{
    struct color *average;
    int row;
    int col;

    average = xcalloc(1, sizeof(struct color));
    for (row = row_start; row < row_end; ++row) {
        DWORD red_sum   = 0;
        DWORD green_sum = 0;
        DWORD blue_sum  = 0;

        for (col = 0; col < bmp->width; ++col) {
            struct color *color;

            color = &bmp->palette[bmp->image[row * bmp->width + col]];
            red_sum   += color->red;
            green_sum += color->green;
            blue_sum  += color->blue;
        }

        average->red   += red_sum   / bmp->width;
        average->green += green_sum / bmp->width;
        average->blue  += blue_sum  / bmp->width;
    }

    return average;
}

static int by_red(const void *index_a_ptr, const void *index_b_ptr)
{
    int index_a = *((int*)index_a_ptr);
    int index_b = *((int*)index_b_ptr);
    struct color *a = &bmp->palette[bmp->image[index_a]];
    struct color *b = &bmp->palette[bmp->image[index_b]];

    return a->red - b->red;
}

static int by_green(const void *index_a_ptr, const void *index_b_ptr)
{
    int index_a = *((int*)index_a_ptr);
    int index_b = *((int*)index_b_ptr);
    struct color *a = &bmp->palette[bmp->image[index_a]];
    struct color *b = &bmp->palette[bmp->image[index_b]];

    return a->green - b->green;
}

static int by_blue(const void *index_a_ptr, const void *index_b_ptr)
{
    int index_a = *((int*)index_a_ptr);
    int index_b = *((int*)index_b_ptr);
    struct color *a = &bmp->palette[bmp->image[index_a]];
    struct color *b = &bmp->palette[bmp->image[index_b]];

    return a->blue - b->blue;
}

struct color *median_cut(int row_start, int row_end, int ncuts)
{
    BYTE *image_offset;
    size_t image_length;
    static struct color *palette;
    static int ncolors;
    int max_range;
    int median;

    printf("                                                                               \r");
    printf("Quantize %3d - %3d - %3d: ", row_start, row_end, ncuts);

    if (ncuts == 0) {
        struct color *average_color;

        ++ncolors;
        palette = xrealloc(palette, sizeof(struct color) * ncolors);
        average_color = get_average_color(row_start, row_end);
        palette[ncolors - 1].red   = average_color->red;
        palette[ncolors - 1].green = average_color->green;
        palette[ncolors - 1].blue  = average_color->blue;

        return palette;
    }

    max_range = get_max_range(row_start, row_end);
    printf("MR=%d, sorting.", max_range);

    image_offset = bmp->image + bmp->width * row_start;
    image_length = bmp->width * (row_end - row_start);

    if (max_range == MAX_RANGE_RED) {
        qsort(image_offset, image_length, 1, by_red);
    } else if (max_range == MAX_RANGE_GREEN) {
        qsort(image_offset, image_length, 1, by_green);
    } else {
        qsort(image_offset, image_length, 1, by_blue);
    }
    printf("done.\r");
    fflush(stdout);

    median = (row_start + row_end) / 2;

    median_cut(row_start, median,  ncuts - 1);
    median_cut(median   , row_end, ncuts - 1);
}

struct color *quantize(struct bitmap *original_bmp, int ncuts)
{
    struct color *palette;

    bmp = bitmap_copy(original_bmp);
    palette = median_cut(0, bmp->height, ncuts);
    bitmap_free(bmp);
    bmp = NULL;

    return palette;
}

