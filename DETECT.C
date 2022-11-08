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

#include "detect.h"

enum graphics_type detect_graphics(void)
{
    union REGS regs = { 0 };

    /* First try int10 service */
    regs.h.ah = 0x1a;
    regs.h.al = 0x00;
    int86(0x10, &regs, &regs);

    if (regs.h.al == 0x1a) {
        switch (regs.h.bl) {
            case 0x00:
                return GRAPHICS_ERROR;

            case 0x01:
                return MDA_GRAPHICS;

            case 0x02:
                return CGA_GRAPHICS;

            case 0x04:
            case 0x05:
                return EGA_GRAPHICS;

            case 0x07:
            case 0x08:
                return VGA_GRAPHICS;
        }

        /* if we had int10 service assume CGA */
        return CGA_GRAPHICS;
    }

    /* If not detected yet check if it's an EGA card */
    regs.h.ah = 0x12;
    regs.x.bx = 0x10;
    int86(0x10, &regs, &regs);

    if (regs.x.bx != 0x10) {
        return EGA_GRAPHICS;
    }

    /* Try equipment info if still not found */
    int86(0x11, &regs, &regs);

    if (((regs.h.al & 0x30) >> 4) == 3) {
        return MDA_GRAPHICS;
    }

    /* if all else fails assume CGA */
    return CGA_GRAPHICS;
}

