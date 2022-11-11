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
    struct color *a = (struct color *) color_a;
    struct color *b = (struct color *) color_b;

    return a->red - b->red;
}

static int sort_by_green(const void *color_a, const void *color_b)
{
    struct color *a = (struct color *) color_a;
    struct color *b = (struct color *) color_b;

    return a->green - b->green;
}

static int sort_by_blue(const void *color_a, const void *color_b)
{
    struct color *a = (struct color *) color_a;
    struct color *b = (struct color *) color_b;

    return a->blue - b->blue;
}

static void sort_palette(struct color *palette, int ncolors, int by)
{
    int (*compare)(const void *, const void *) = NULL;

    if (by == MAX_RANGE_RED) {
        compare = sort_by_red;
    } else if (by == MAX_RANGE_GREEN) {
        compare = sort_by_green;
    } else {
        compare = sort_by_blue;
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
    struct color *new_bucket;
    int median;

    /*
     * if done add the average color of the bucket
     * to the reduced palette.
     */
    if (ncuts == 0) {
        struct color *average_color = &reduced[*nreduced];

        palette_average(palette, ncolors, average_color);
        ++(*nreduced);
        return;
    }

    sort_palette(palette, ncolors, max_range(palette, ncolors));

    median = (ncolors + 1) / 2;
    new_bucket = &palette[median];

    median_cut(palette,    median,           ncuts - 1, reduced, nreduced);
    median_cut(new_bucket, ncolors - median, ncuts - 1, reduced, nreduced);
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