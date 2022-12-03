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

#include <string.h>

#include "bitmap.h"
#include "color.h"
#include "dither.h"

#define INDEX(x, y) ((y) * bmp->width + (x))

/*
 * Clamp a value between 0 and 255, inclusive.
 */
static BYTE clamp(int value)
{
    if (value > 255) {
        return 255;
    }

    if (value < 0) {
        return 0;
    }

    return value;
}

/*
 * Convert bitmap to grayscale with dithering in place.
 */
void grayscale_dither(struct bitmap *bmp, int ncolors)
{
    int error[2][MAX_IMAGE_WIDTH];
    size_t error_size = sizeof(int) * 2 * MAX_IMAGE_WIDTH;
    int row;
    int col;

    memset(error, 0, error_size);
    for (row = 0; row < bmp->height - 1; ++row) {
        printf("Dithering %3d%%\r", row * 100 / bmp->height);
        fflush(stdout);
        for (col = 1; col < bmp->width - 1; ++col) {
            struct color *color_ptr;
	    int luma_error;
            BYTE old_pixel;
            BYTE new_pixel;

            color_ptr = &bmp->palette[bmp->image[INDEX(col, row)]];
            old_pixel = clamp(color_to_luma(color_ptr) + error[0][col]);
            new_pixel = (old_pixel * ncolors / 256) * (256 / ncolors);
            bmp->image[INDEX(col, row)] = new_pixel;

	    luma_error = old_pixel - new_pixel;
            error[0][col + 1] += luma_error * 7 / 16;
            error[1][col - 1] += luma_error * 3 / 16;
            error[1][col    ] += luma_error * 5 / 16;
            error[1][col + 1] += luma_error * 1 / 16;
        }

        memcpy(&error[0][0], &error[1][0], error_size / 2);
        memset(&error[1][0], 0, error_size / 2);
    }
}

/*
 * Dither a bitmap in place
 */
struct error_color {
    int red;
    int green;
    int blue;
};

void dither(struct bitmap *bmp, struct color *palette, int ncolors)
{
    struct error_color error[2][MAX_IMAGE_WIDTH];
    size_t error_size = sizeof(struct error_color) * 2 * MAX_IMAGE_WIDTH;
    int row;
    int col;

    memset(error, 0, error_size);
    for (row = 0; row < bmp->height - 1; ++row) {
        printf("Dithering %3d%%\r", row * 100 / bmp->height);
        fflush(stdout);
        for (col = 1; col < bmp->width - 1; ++col) {
            struct color old_pixel;
            struct color new_pixel;
            struct color *color_ptr;
            int palette_index;
	    int red_error;
	    int green_error;
	    int blue_error;

            color_ptr = &bmp->palette[bmp->image[INDEX(col, row)]];
            old_pixel.red   = clamp(color_ptr->red   + error[0][col].red);
            old_pixel.green = clamp(color_ptr->green + error[0][col].green);
            old_pixel.blue  = clamp(color_ptr->blue  + error[0][col].blue);

            palette_index = find_closest_color(&old_pixel, palette, ncolors);
            bmp->image[INDEX(col, row)] = palette_index;

            color_ptr = &palette[palette_index];
            new_pixel.red   = color_ptr->red;
            new_pixel.green = color_ptr->green;
            new_pixel.blue  = color_ptr->blue;

	    red_error   = old_pixel.red   - new_pixel.red;
	    green_error = old_pixel.green - new_pixel.green;
	    blue_error  = old_pixel.blue  - new_pixel.blue;

            error[0][col + 1].red   += red_error   * 7 / 16;
            error[0][col + 1].green += green_error * 7 / 16;
            error[0][col + 1].blue  += blue_error  * 7 / 16;

            error[1][col - 1].red   += red_error   * 3 / 16;
            error[1][col - 1].green += green_error * 3 / 16;
            error[1][col - 1].blue  += blue_error  * 3 / 16;

            error[1][col    ].red   += red_error   * 5 / 16;
            error[1][col    ].green += green_error * 5 / 16;
            error[1][col    ].blue  += blue_error  * 5 / 16;

            error[1][col + 1].red   += red_error   * 1 / 16;
            error[1][col + 1].green += green_error * 1 / 16;
            error[1][col + 1].blue  += blue_error  * 1 / 16;
        }

        memcpy(&error[0][0], &error[1][0], error_size / 2);
        memset(&error[1][0], 0, error_size / 2);
    }
}

