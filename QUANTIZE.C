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
static int get_max_range(WORD row_start, WORD row_end)
{
    BYTE r_max = 0;
    BYTE g_max = 0;
    BYTE b_max = 0;
    WORD row, col;

    for (row = row_start; row < row_end; ++row) {
        for (col = 0; col < bmp->width; ++col) {
            struct rgb *color;

            color = &bmp->palette[bmp->image[row * bmp->width + col]];
            if (color->r > r_max) {
                r_max = color->r;
            }

            if (color->g > g_max) {
                g_max = color->g;
            }

            if (color->b > b_max) {
                b_max = color->b;
            }

        }
    }

    if (r_max > g_max && r_max > b_max) {
        return MAX_RANGE_RED;
    }

    if (g_max > r_max && g_max > b_max) {
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
static struct rgb *get_average_color(int row_start, int row_end)
{
    struct rgb *average_color;
    DWORD red_average = 0;
    DWORD green_average = 0;
    DWORD blue_average = 0;
    DWORD ncolors = 0;
    WORD row, col;

    for (row = row_start; row < row_end; ++row) {
        for (col = 0; col < bmp->width; ++col) {
            struct rgb *color =
                &bmp->palette[bmp->image[row * bmp->width + col]];

            red_average   = (color->r * 8 + ncolors * red_average)
                          / (ncolors + 1);

            green_average = (color->g * 8 + ncolors * green_average)
                          / (ncolors + 1);

            blue_average  = (color->b * 8 + ncolors * blue_average)
                          / (ncolors + 1);

            ++ncolors;
        }
    }

    average_color = xmalloc(sizeof(struct rgb));

    average_color->r = (red_average + 4) / 8;
    average_color->g = (green_average + 4) / 8;
    average_color->b = (blue_average + 4) / 8;

    return average_color;
}

/*
 * Sort pixels by a color component.
 */
static int by_red(const void *index_a_ptr, const void *index_b_ptr)
{
    BYTE index_a = *((BYTE *) index_a_ptr);
    BYTE index_b = *((BYTE *) index_b_ptr);
    struct rgb *a = &bmp->palette[index_a];
    struct rgb *b = &bmp->palette[index_b];

    return a->r - b->r;
}

static int by_green(const void *index_a_ptr, const void *index_b_ptr)
{
    BYTE index_a = *((BYTE *) index_a_ptr);
    BYTE index_b = *((BYTE *) index_b_ptr);
    struct rgb *a = &bmp->palette[index_a];
    struct rgb *b = &bmp->palette[index_b];

    return a->g - b->g;
}

static int by_blue(const void *index_a_ptr, const void *index_b_ptr)
{
    BYTE index_a = *((BYTE *) index_a_ptr);
    BYTE index_b = *((BYTE *) index_b_ptr);
    struct rgb *a = &bmp->palette[index_a];
    struct rgb *b = &bmp->palette[index_b];

    return a->b - b->b;
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
static void median_cut(WORD row_start, WORD row_end, int ncuts,
                       struct rgb **palette, int *ncolors)
{
    BYTE *image_offset;
    WORD image_length;
    int max_range;
    int median;

    printf("Quantizing\r");

    /*
     * We are done for this bucket, take the average color
     * and add it to the palette.
     */
    if (ncuts == 0) {
        struct rgb *average_color;
        struct rgb *palette_entry;
        int palette_index;

        palette_index = *ncolors;
        ++(*ncolors);
        *palette = xrealloc(*palette, sizeof(struct rgb) * *ncolors);
        average_color = get_average_color(row_start, row_end);
        palette_entry = &(*palette)[palette_index];
        palette_entry->r = average_color->r;
        palette_entry->g = average_color->g;
        palette_entry->b = average_color->b;
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

    median_cut(row_start, median, ncuts - 1, palette, ncolors);
    median_cut(median, row_end, ncuts - 1, palette, ncolors);
}

/*
 * Median cut driver.
 */
struct rgb *quantize(struct bitmap *original_bmp, int ncuts)
{
    struct rgb *palette = NULL;
    int ncolors = 0;
    int i;

    bmp = bitmap_copy(original_bmp);
    median_cut(0, bmp->height, ncuts, &palette, &ncolors);
    bitmap_free(bmp);
    bmp = NULL;

    return palette;
}
