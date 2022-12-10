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

/* XXX: this needs a large stack otherwise we run out of it. */
extern unsigned _stklen = 1024 * 62;

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
    struct color *average_color;
    DWORD red_sum   = 0;
    DWORD green_sum = 0;
    DWORD blue_sum  = 0;
    DWORD ncolors   = 0;
    int row;
    int col;

    for (row = row_start; row < row_end; ++row) {
        for (col = 0; col < bmp->width; ++col) {
            struct color *color;

            color = &bmp->palette[bmp->image[row * bmp->width + col]];
            red_sum   += color->red;
            green_sum += color->green;
            blue_sum  += color->blue;
            ++ncolors;
        }
    }

    average_color = xmalloc(sizeof(struct color));

    average_color->red   = red_sum   / ncolors;
    average_color->green = green_sum / ncolors;
    average_color->blue  = blue_sum  / ncolors;

    return average_color;
}

static struct color *get_average_color_x(int row_start, int row_end)
{
    struct color *average_color;
    DWORD red_average   = 0;
    DWORD green_average = 0;
    DWORD blue_average  = 0;
    DWORD n = 0;
    int row;
    int col;

    for (row = row_start; row < row_end; ++row) {
        for (col = 0; col < bmp->width; ++col) {
            struct color *color;

            color = &bmp->palette[bmp->image[row * bmp->width + col]];
            red_average   = (color->red   + n * red_average)   / (n + 1);
            green_average = (color->green + n * green_average) / (n + 1);
            blue_average  = (color->blue  + n * blue_average)  / (n + 1);
            ++n;
        }
    }

    average_color = xmalloc(sizeof(struct color));

    average_color->red   = red_average;
    average_color->green = green_average;
    average_color->blue  = blue_average;

    return average_color;
}

static int by_red(const void *index_a_ptr, const void *index_b_ptr)
{
    BYTE index_a = *((BYTE *)index_a_ptr);
    BYTE index_b = *((BYTE *)index_b_ptr);
    struct color *a = &bmp->palette[index_a];
    struct color *b = &bmp->palette[index_b];

    return a->red - b->red;
}

static int by_green(const void *index_a_ptr, const void *index_b_ptr)
{
    BYTE index_a = *((BYTE *)index_a_ptr);
    BYTE index_b = *((BYTE *)index_b_ptr);
    struct color *a = &bmp->palette[index_a];
    struct color *b = &bmp->palette[index_b];

    return a->green - b->green;
}

static int by_blue(const void *index_a_ptr, const void *index_b_ptr)
{
    BYTE index_a = *((BYTE *)index_a_ptr);
    BYTE index_b = *((BYTE *)index_b_ptr);
    struct color *a = &bmp->palette[index_a];
    struct color *b = &bmp->palette[index_b];

    return a->blue - b->blue;
}

struct color *median_cut(int row_start, int row_end, int ncuts,
                         struct color **palette, int *ncolors)
{
    BYTE *image_offset;
    size_t image_length;
    int max_range;
    int median;

    printf("Quantize %3d - %3d\n", row_start, row_end);

    if (ncuts == 0) {
        struct color *average_color;
        struct color *palette_entry;
        int i;

        i = *ncolors;
        ++(*ncolors);
        *palette = xrealloc(*palette, sizeof(struct color) * *ncolors);
        average_color = get_average_color(row_start, row_end);
        palette_entry = &(*palette)[i];
        palette_entry->red   = average_color->red;
        palette_entry->green = average_color->green;
        palette_entry->blue  = average_color->blue;
        free(average_color);

        printf("  adding color #%02x%02x%02x\n", palette_entry->red, palette_entry->green, palette_entry->blue);
        return *palette;
    }

    max_range = get_max_range(row_start, row_end);

    image_offset = bmp->image + bmp->width * row_start;

    /*
     * XXX: size_t is 2 bytes, signed so this overflows for 'larger'
     *      bitmaps..
     */
    image_length = bmp->width * (row_end - row_start);

    printf("  image base is at %p.\n", bmp->image);
    if (max_range == MAX_RANGE_RED) {
        printf("  sorting by red at %p, len %d.\n", image_offset, image_length);
        qsort(image_offset, image_length, 1, by_red);
    } else if (max_range == MAX_RANGE_GREEN) {
        printf("  sorting by green at %p, len %d.\n", image_offset, image_length);
        qsort(image_offset, image_length, 1, by_green);
    } else {
        printf("  sorting by blue at %p, len %d.\n", image_offset, image_length);
        qsort(image_offset, image_length, 1, by_blue);
    }

    median = (row_start + row_end) / 2;

    printf("  median is at %d, redoing for bottom and top part now.\n", median);
    median_cut(row_start, median,  ncuts - 1, palette, ncolors);
    median_cut(median   , row_end, ncuts - 1, palette, ncolors);
}

struct color *quantize(struct bitmap *original_bmp, int ncuts)
{
    struct color *palette = NULL;
    int ncolors = 0;
    int i;

    bmp = bitmap_copy(original_bmp);
    median_cut(0, bmp->height, ncuts, &palette, &ncolors);
    bitmap_free(bmp);
    bmp = NULL;

    return palette;
}

