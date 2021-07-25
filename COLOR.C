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

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system.h"
#include "array.h"
#include "color.h"

struct array *palette_to_array(BYTE * palette, int palette_size)
{
    struct array *result = array_new();
    size_t i;

    for (i = 0; i < palette_size; ++i) {
        struct color *color = xmalloc(sizeof(*color));
        int offset = i * 3;

        color->red = palette[offset + 0];
        color->green = palette[offset + 1];
        color->blue = palette[offset + 2];

        array_add(result, color);
    }

    return result;
}


/* which part has the maximum val, r, g or b? */
static int max_color(struct array *palette)
{
    size_t i;
    BYTE red_max = 0;
    BYTE green_max = 0;
    BYTE blue_max = 0;

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

static void sort_palette(struct array *palette, int by)
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
static struct color *palette_average(struct array *palette)
{
    struct color *average_color = xmalloc(sizeof(*average_color));
    size_t i;
    DWORD red_sum = 0;
    DWORD green_sum = 0;
    DWORD blue_sum = 0;

    for (i = 0; i < palette->size; ++i) {
        struct color *color = palette->items[i];

        red_sum += color->red;
        green_sum += color->green;
        blue_sum += color->blue;
    }

    average_color->red = red_sum / palette->size;
    average_color->green = green_sum / palette->size;
    average_color->blue = blue_sum / palette->size;

    return average_color;
}

void median_cut(struct array *palette, int ncuts, struct array *reduced)
{
    struct array *top = NULL;
    struct array *bottom = NULL;
    size_t median;
    size_t i;

    /*
     * if done add the average color of the bucket
     * to the reduced palette.
     */
    if (ncuts == 0) {
        struct color *average_color = palette_average(palette);

        array_add(reduced, average_color);
        return;
    }

    /* sort palette by highest color val of r,g,b */
    sort_palette(palette, max_color(palette));

    /* split into top and bottom part */
    median = palette->size >> 1;
    bottom = array_new();
    top = array_new();

    for (i = 0; i < median; ++i) {
        array_add(bottom, palette->items[i]);
    }

    for (i = median; i < palette->size; ++i) {
        array_add(top, palette->items[i]);
    }

    /* repeat for the two created buckets */
    median_cut(bottom, ncuts - 1, reduced);
    median_cut(top, ncuts - 1, reduced);

    array_free(bottom);
    array_free(top);
}

static void print_binary(FILE *output_file, BYTE value)
{
    int bit;

    for (bit = 0; bit < 8; ++bit) {
        int bitmask = (1 << (7 - bit));

        if (value & bitmask) {
            fputc('1', output_file);
        }
        else {
            fputc('0', output_file);
        }
    }
}

void write_palette(struct array *palette, char *filename)
{
    FILE *output_file = fopen(filename, "w");
    size_t i;

    if (output_file == NULL) {
        xerror("write_palette: can't open file.");
    }

    for (i = 0; i < palette->size; ++i) {
        struct color *color = palette->items[i];
        BYTE ega_red   = color->red   >> 6;
        BYTE ega_green = color->green >> 6;
        BYTE ega_blue  = color->blue  >> 6;

        fprintf(output_file,
                "#%02x%02x%02x => #%02x%02x%02x => %d %d %d => %02x => ",
                color->red, color->green, color->blue,
                ega_red * 85, ega_green * 85, ega_blue * 85,
                ega_red, ega_green, ega_blue,
                ega_make_color(color));
        print_binary(output_file, ega_make_color(color));
        fputc('\n', output_file);
    }

    fclose(output_file);
}

