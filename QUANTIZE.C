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

/* a private copy of a bitmap to work on */
static struct bitmap *bmp;

/*
 * See which color component has the largest range.
 */
static int get_max_range(int row_start, int row_end)
{
    BYTE red_max   = 0;
    BYTE green_max = 0;
    BYTE blue_max  = 0;
    int row;
    int column;

    for (row = row_start; row < row_end; ++row) {
        for (column = 0; column < bmp->width; ++column) {
            struct color *color;

            color = &bmp->palette[bmp->image[row * bmp->width + column]];
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

/*
 * Calculate a running average for the pixel colors between
 * row_start and row_end.
 * Scale color components by 8 to have a low precision fixed-point
 * value which we can round back in the end.
 */
static struct color *get_average_color(int row_start, int row_end)
{
    struct color *average_color;
    DWORD red_average   = 0;
    DWORD green_average = 0;
    DWORD blue_average  = 0;
    DWORD ncolors       = 0;
    int row;
    int column;

    for (row = row_start; row < row_end; ++row) {
        for (column = 0; column < bmp->width; ++column) {
            struct color *color;

            color = &bmp->palette[bmp->image[row * bmp->width + column]];
            red_average   = (color->red   * 8 + ncolors * red_average)
			  / (ncolors + 1);
            green_average = (color->green * 8 + ncolors * green_average)
                          / (ncolors + 1);
            blue_average  = (color->blue  * 8 + ncolors * blue_average)
                          / (ncolors + 1);
            ++ncolors;
        }
    }

    average_color = xmalloc(sizeof(struct color));

    average_color->red   = (red_average   + 4) / 8;
    average_color->green = (green_average + 4) / 8;
    average_color->blue  = (blue_average  + 4) / 8;

    return average_color;
}

/*
 * Sort pixels by a color component.
 */
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

/*
 * Do a median-cut to generate an optimal palette. This is
 * very slow with regard to sorting all the pixels under
 * Turbo C.
 *
 * The selected colors are ok but further mapping to a
 * fixed palette makes some pretty far off, this needs
 * more work.
 */
static void median_cut(int row_start, int row_end, int ncuts,
                       struct color **palette, int *ncolors)
{
    BYTE *image_offset;
    size_t image_length;
    int max_range;
    int median;

    printf("Quantizing\r");

    /*
     * We are done for this bucket, take the average color
     * and add it to the palette.
     */
    if (ncuts == 0) {
        struct color *average_color;
        struct color *palette_entry;
        int palette_index;

        palette_index = *ncolors;
        ++(*ncolors);
        *palette = xrealloc(*palette, sizeof(struct color) * *ncolors);
        average_color = get_average_color(row_start, row_end);
        palette_entry = &(*palette)[palette_index];
        palette_entry->red   = average_color->red;
        palette_entry->green = average_color->green;
        palette_entry->blue  = average_color->blue;
        free(average_color);
        return;
    }

    max_range = get_max_range(row_start, row_end);

    image_offset = bmp->image + bmp->width * row_start;
    image_length = bmp->width * (row_end - row_start);

    switch (max_range) {
        case MAX_RANGE_RED:
            qsort(image_offset, image_length, 1, by_red);
            break;

        case MAX_RANGE_GREEN:
            qsort(image_offset, image_length, 1, by_green);
            break;

        case MAX_RANGE_BLUE:
            qsort(image_offset, image_length, 1, by_blue);
            break;

        default:
            xerror("median_cut: illegal max_range.");
    }

    median = (row_start + row_end) / 2;

    median_cut(row_start, median,  ncuts - 1, palette, ncolors);
    median_cut(median   , row_end, ncuts - 1, palette, ncolors);
}

/*
 * Median cut driver.
 */
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

