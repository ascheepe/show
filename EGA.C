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

#include "system.h"
#include "ega.h"
#include "vector.h"
#include "color.h"

static BYTE *vmem = (BYTE *) 0xA0000000L;

/*
 * EGA stores colors as
 * 7 6 5 4 3 2 1 0
 * | | | | | | | +- Blue  MSB
 * | | | | | | +--- Green MSB
 * | | | | | +----- Red   MSB
 * | | | | +------- Blue  LSB
 * | | | +--------- Green LSB
 * | | +----------- Red   LSB
 * | +------------- Reserved
 * +--------------- Reserved
 */
BYTE ega_make_color(struct color *color)
{
    BYTE red = color->red >> 6;
    BYTE green = color->green >> 6;
    BYTE blue = color->blue >> 6;

    BYTE red_msb = red >> 1;
    BYTE green_msb = green >> 1;
    BYTE blue_msb = blue >> 1;

    BYTE red_lsb = red & 1;
    BYTE green_lsb = green & 1;
    BYTE blue_lsb = blue & 1;

    return (blue_msb << 0) | (green_msb << 1) | (red_msb << 2) |
           (blue_lsb << 3) | (green_lsb << 4) | (red_lsb << 5);
}

/*
 * Attribute registers are accessed via I/O port 0x3c0.
 * This register can act like a flip-flop to automatically
 * switch between setting the palette index and the palette
 * value. To enable this mode a read of port 0x3da is
 * required. The datasheet calls this 'sending an IOR command'.
 *
 * After that the first write selects the attribute
 * and the second sets the value.
 *
 * Attributes 0x00 - 0x0F specify the 16 color palette in
 * the format as the make_color function provides.
 */
void ega_set_palette(struct vector *palette)
{
    int i;

    /* enable 0x3c0 flip-flop */
    (void) inp(0x3da);

    /* and set the palette */
    for (i = 0; i < palette->size; ++i) {
        struct color *color = palette->items[i];

        outp(0x3c0, i);
        outp(0x3c0, ega_make_color(color));
    }
}

void ega_plot(int x, int y, int color)
{
    BYTE *pixel = vmem + (y << 5) + (y << 3) + (x >> 3);
    BYTE mask = 0x80 >> (x & 7);

    /*
     * color selects which planes to write to
     * this is the palette index value
     * e.g. 11 is cyan with default palette.
     */
    outp(0x3c4, 2);
    outp(0x3c5, color);

    /* set pixel mask */
    outp(0x3ce, 8);
    outp(0x3cf, mask);

    /*
     * with the mask set above we can just
     * write all 1's
     */
    *pixel |= 0xff;
}

void ega_clear_screen(void)
{
    set_mode(MODE_EGA);
    /* memset(vmem, 0, 128 * 1024); */
}

