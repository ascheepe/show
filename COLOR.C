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

#if 0
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
                struct color *reduced)
{
    /* 1. find the greatest range of each channel
     * 2. sort palette by that channel
     * 3. take the color at the median of it
     * 4. split into colors above the median color and below.
     *    above means where the largest range channel is higher/lower than it.
     * 5. repeat for these two buckets
     *
     * XXX: below is not working code!
     */

    struct color *above_median;
    struct color *below_median;
    size_t median_index;
    size_t i;
    BYTE median;

    /*
     * if done add the average color of the bucket
     * to the reduced palette.
     */
    if (ncuts == 0) {
        palette_average(palette, ncolors, reduced);
        printf("#%02x%02x%02x\n", reduced->red,
                                  reduced->green,
                                  reduced->blue);
        ++reduced;
        free(palette);
        return;
    }

    /* sort palette by highest channel range */
    sort_palette(palette, ncolors, max_range(palette, ncolors));

    /* split into top and bottom part */
    median_index = (ncolors + 1) / 2;
    if ((median_index & 1) == 0) {
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

    above_median = calloc(ncolors / 2, sizeof(struct color));
    below_median = calloc(ncolors / 2, sizeof(struct color));

    for (i = 0; i < median; ++i) {
        below_median[i].red   = palette[i].red;
        below_median[i].green = palette[i].green;
        below_median[i].blue  = palette[i].blue;
    }

    for (i = median; i < ncolors; ++i) {
        above_median[i - median].red   = palette[i].red;
        above_median[i - median].green = palette[i].green;
        above_median[i - median].blue  = palette[i].blue;
    }

    /* repeat for the two buckets */
    median_cut(below_median, ncolors - median, ncuts - 1, reduced);
    median_cut(above_median, ncolors - median, ncuts - 1, reduced);
}
#endif

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