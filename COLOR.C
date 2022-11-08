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

#include "system.h"
#include "vector.h"
#include "color.h"

struct vector *palette_to_vector(BYTE *palette, int size)
{
    struct vector *result;
    size_t i;

    result = vector_new();

    for (i = 0; i < size; ++i) {
        struct color *color;
        int offset = i * 3;

        color = xmalloc(sizeof(*color));
        color->red   = palette[offset + 0];
        color->green = palette[offset + 1];
        color->blue  = palette[offset + 2];

        vector_add(result, color);
    }

    return result;
}


/* which part has the maximum value, red, green or blue? */
static int max_color(struct vector *palette)
{
    BYTE red_max   = 0;
    BYTE green_max = 0;
    BYTE blue_max  = 0;
    size_t i;

    for (i = 0; i < palette->size; ++i) {
        struct color *color = palette->items[i];

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

    if (red_max > green_max && red_max > blue_max) {
        return MAX_COLOR_RED;
    }

    if (green_max > red_max && green_max > blue_max) {
        return MAX_COLOR_GREEN;
    }

    return MAX_COLOR_BLUE;
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

static void sort_palette(struct vector *palette, int by)
{
    int (*compare)(const void *, const void *) = NULL;

    switch (by) {
        case MAX_COLOR_RED:
            compare = sort_by_red;
            break;

        case MAX_COLOR_GREEN:
            compare = sort_by_green;
            break;

        case MAX_COLOR_BLUE:
            compare = sort_by_blue;
            break;

        default:
            xerror("sort_palette: unknown sort function.");
    }

    qsort(palette->items, palette->size, sizeof(void *), compare);
}

/* average color of palette */
static struct color *palette_average(struct vector *palette)
{
    struct color *average_color;
    DWORD red_sum   = 0;
    DWORD green_sum = 0;
    DWORD blue_sum  = 0;
    size_t i;

    average_color = xmalloc(sizeof(*average_color));

    for (i = 0; i < palette->size; ++i) {
        struct color *color = palette->items[i];

        red_sum   += color->red;
        green_sum += color->green;
        blue_sum  += color->blue;
    }

    average_color->red   = red_sum   / palette->size;
    average_color->green = green_sum / palette->size;
    average_color->blue  = blue_sum  / palette->size;

    return average_color;
}

void median_cut(struct vector *palette, int ncuts, struct vector *reduced)
{
    struct vector *top = NULL;
    struct vector *bottom = NULL;
    size_t median;
    size_t i;

    /*
     * if done add the average color of the bucket
     * to the reduced palette.
     */
    if (ncuts == 0) {
        struct color *average_color;

        average_color = palette_average(palette);
        vector_add(reduced, average_color);
        return;
    }

    /* sort palette by highest color val of r,g,b */
    sort_palette(palette, max_color(palette));

    /* split into top and bottom part */
    median = palette->size >> 1;
    bottom = vector_new();
    top    = vector_new();

    for (i = 0; i < median; ++i) {
        vector_add(bottom, palette->items[i]);
    }

    for (i = median; i < palette->size; ++i) {
        vector_add(top, palette->items[i]);
    }

    /* repeat for the two created buckets */
    median_cut(bottom, ncuts - 1, reduced);
    median_cut(top, ncuts - 1, reduced);

    vector_free(bottom);
    vector_free(top);
}

#if 0
static void print_binary(FILE *f, BYTE value)
{
    int bit;

    for (bit = 0; bit < 8; ++bit) {
        int mask = (1 << (7 - bit));

        if (value & mask) {
            fputc('1', f);
        } else {
            fputc('0', f);
        }
    }
}

void write_palette(struct vector *palette, char *filename)
{
    FILE *palette_file;
    size_t i;

    palette_file = fopen(filename, "w");
    if (palette_file == NULL) {
        xerror("write_palette: can't open file.");
    }

    for (i = 0; i < palette->size; ++i) {
        struct color *color = palette->items[i];
        BYTE ega_red = color->red >> 6;
        BYTE ega_green = color->green >> 6;
        BYTE ega_blue = color->blue >> 6;

        fprintf(palette_file,
                "#%02x%02x%02x => #%02x%02x%02x => %d %d %d => %02x => ",
                color->red, color->green, color->blue,
                ega_red * 85, ega_green * 85, ega_blue * 85,
                ega_red, ega_green, ega_blue, ega_make_color(color));
        print_binary(palette_file, ega_make_color(color));
        fputc('\n', palette_file);
    }

    fclose(palette_file);
}
#endif

BYTE color_to_luma(struct color *color)
{
    return  3 * color->red   /  10
         + 59 * color->green / 100
         + 11 * color->blue  / 100;
}

#define SQR(n) ((n)*(n))

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
        long red_diff   = color->red   - palette_color->red;
        long green_diff = color->green - palette_color->green;
        long blue_diff  = color->blue  - palette_color->blue;

        distance = SQR(red_diff) + SQR(green_diff) + SQR(blue_diff);
        if (distance < max_distance) {
            max_distance = distance;
            match = i;
        }
    }

    return match;
}

