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

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "system.h"
#include "color.h"

/* which channel has the greatest range? */
static int max_range(struct color *palette, int ncolors)
{
    BYTE red_min     = 255;
    BYTE green_min   = 255;
    BYTE blue_min    = 255;
    BYTE red_max     = 0;
    BYTE green_max   = 0;
    BYTE blue_max    = 0;
    BYTE range_red   = 0;
    BYTE range_green = 0;
    BYTE range_blue  = 0;
    size_t i;

    for (i = 0; i < ncolors; ++i) {
        struct color *color = &palette[i];

        if (color->red > red_max) {
            red_max = color->red;
        }
        if (color->red < red_min) {
            red_min = color->red;
        }

        if (color->green > green_max) {
            green_max = color->green;
        }
        if (color->green < green_min) {
            green_min = color->green;
        }

        if (color->blue > blue_max) {
            blue_max = color->blue;
        }
        if (color->blue < blue_min) {
            blue_min = color->blue;
        }
    }

    range_red   = red_max   - red_min;
    range_green = green_max - green_min;
    range_blue  = blue_max  - blue_min;

    if (range_red > range_green && range_red > range_blue) {
        return MAX_RANGE_RED;
    }

    if (range_green > range_red && range_green > range_blue) {
        return MAX_RANGE_GREEN;
    }

    return MAX_RANGE_BLUE;
}

static int sort_by_red(const void *color_a, const void *color_b)
{
    struct color *a = *((struct color **) color_a);
    struct color *b = *((struct color **) color_b);

    return a->red - b->red;
}

static int sort_by_green(const void *color_a, const void *color_b)
{
    struct color *a = *((struct color **) color_a);
    struct color *b = *((struct color **) color_b);

    return a->green - b->green;
}

static int sort_by_blue(const void *color_a, const void *color_b)
{
    struct color *a = *((struct color **) color_a);
    struct color *b = *((struct color **) color_b);

    return a->blue - b->blue;
}

static void sort_palette(struct color *palette, int ncolors, int by)
{
    int (*compare)(const void *, const void *) = NULL;

    switch (by) {
        case MAX_RANGE_RED:
            compare = sort_by_red;
            break;

        case MAX_RANGE_GREEN:
            compare = sort_by_green;
            break;

        case MAX_RANGE_BLUE:
            compare = sort_by_blue;
            break;

        default:
            xerror("sort_palette: unknown sort function.");
    }

    qsort(palette, ncolors, sizeof(struct color), compare);
}

/* average color of palette */
static void palette_average(struct color *palette, int ncolors,
                            struct color *average_color)
{
    DWORD red_sum   = 0;
    DWORD green_sum = 0;
    DWORD blue_sum  = 0;
    size_t i;

    for (i = 0; i < ncolors; ++i) {
        red_sum   += palette[i].red;
        green_sum += palette[i].green;
        blue_sum  += palette[i].blue;
    }

    average_color->red   = red_sum   / ncolors;
    average_color->green = green_sum / ncolors;
    average_color->blue  = blue_sum  / ncolors;
}

void median_cut(struct color *palette, int ncolors, int ncuts,
                struct color *reduced, int *nreduced)
{
    struct color *above_median;
    struct color *below_median;
    struct color median;
    int greatest_range;
    int median_index;
    int nbelow;
    int nabove;
    int i;

    /* There might be no colors in a bucket */
    if (ncolors == 0) {
        return;
    }

    /*
     * if done add the average color of the bucket
     * to the reduced palette.
     */
    if (ncuts == 0) {
        struct color *average_color = &reduced[*nreduced];

        palette_average(palette, ncolors, average_color);
#if 0
        printf("reduced[%3d] = #%02x%02x%02x\n",
               *nreduced,
               average_color->red,
               average_color->green,
               average_color->blue);
#endif
        free(palette);
        ++(*nreduced);
        return;
    }
#if 0
    printf("median_cut(%p, %3d, %d, %p, %3d)\n",
           palette, ncolors, ncuts, reduced, *nreduced);
#endif
    /*
     * 1. find the greatest range of each channel
     * 2. sort palette by that channel
     */
    greatest_range = max_range(palette, ncolors);
    sort_palette(palette, ncolors, greatest_range);
#if 0
    printf("  greatest_range = %d\n", greatest_range);
#endif
    /*
     * 3. take the color at the median of it
     */
    median_index = (ncolors + 1) / 2;
#if 0
    printf("  median_index   = %d\n", median_index);
#endif
    if ((median_index % 2) == 0) {
        median.red   = (palette[median_index - 1].red +
                        palette[median_index].red) / 2;
        median.green = (palette[median_index - 1].green +
                        palette[median_index].green) / 2;
        median.blue  = (palette[median_index - 1].blue +
                        palette[median_index].blue) / 2;
    } else {
        median.red   = palette[median_index].red;
        median.green = palette[median_index].green;
        median.blue  = palette[median_index].blue;
    }

    /*
     * 4. split into colors above the median color and below.
     */
    above_median = xcalloc(ncolors, sizeof(struct color));
    below_median = xcalloc(ncolors, sizeof(struct color));
    nbelow = 0;
    nabove = 0;

    for (i = 0; i < ncolors; ++i) {
        switch (greatest_range) {
            case MAX_RANGE_RED:
                if (palette[i].red < median.red) {
                    below_median[nbelow].red   = palette[i].red;
                    below_median[nbelow].green = palette[i].green;
                    below_median[nbelow].blue  = palette[i].blue;
                    ++nbelow;
                } else {
                    above_median[nabove].red   = palette[i].red;
                    above_median[nabove].green = palette[i].green;
                    above_median[nabove].blue  = palette[i].blue;
                    ++nabove;
                }
                break;

            case MAX_RANGE_GREEN:
                if (palette[i].green < median.green) {
                    below_median[nbelow].red   = palette[i].red;
                    below_median[nbelow].green = palette[i].green;
                    below_median[nbelow].blue  = palette[i].blue;
                    ++nbelow;
                } else {
                    above_median[nabove].red   = palette[i].red;
                    above_median[nabove].green = palette[i].green;
                    above_median[nabove].blue  = palette[i].blue;
                    ++nabove;
                }
                break;

            case MAX_RANGE_BLUE:
                if (palette[i].blue < median.blue) {
                    below_median[nbelow].red   = palette[i].red;
                    below_median[nbelow].green = palette[i].green;
                    below_median[nbelow].blue  = palette[i].blue;
                    ++nbelow;
                } else {
                    above_median[nabove].red   = palette[i].red;
                    above_median[nabove].green = palette[i].green;
                    above_median[nabove].blue  = palette[i].blue;
                    ++nabove;
                }
                break;
        }
    }
#if 0
    printf("  nabove         = %d\n", nabove);
    printf("  nbelow         = %d\n", nbelow);
#endif
    below_median = xrealloc(below_median, sizeof(struct color) * nbelow);
    above_median = xrealloc(above_median, sizeof(struct color) * nabove);

    /* repeat for the two buckets */
#if 0
    printf("  above_median   = %p\n", above_median);
    printf("  below_median   = %p\n", below_median);
#endif
    median_cut(below_median, nbelow, ncuts - 1, reduced, nreduced);
    median_cut(above_median, nabove, ncuts - 1, reduced, nreduced);
}

BYTE color_to_luma(struct color *color)
{
    return color->red   *  3 /  10
         + color->green * 59 / 100
         + color->blue  * 11 / 100;
}

#define SQR(n) ((DWORD)((n)*(n)))

int find_closest_color(const struct color *color,
                       const struct color *palette,
                       int ncolors)
{
    DWORD max_distance = -1;
    DWORD distance;
    int match;
    int i;

    for (i = 0; i < ncolors; ++i) {
        const struct color *palette_color = &palette[i];
        WORD red_diff   = abs(color->red   - palette_color->red);
        WORD green_diff = abs(color->green - palette_color->green);
        WORD blue_diff  = abs(color->blue  - palette_color->blue);

        distance = SQR(red_diff) + SQR(green_diff) + SQR(blue_diff);
        if (distance < max_distance) {
            max_distance = distance;
            match = i;
        }
    }

    return match;
}

