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
#include <math.h>
#include <dos.h>
#include <conio.h>

#include "color.h"
#include "system.h"
#include "vga.h"

#define VGA_DAC_PEL_ADDRESS   0x3c8
#define VGA_DAC               0x3c9
#define VIDEO_STATUS_REGISTER 0x3da

BYTE *vga_vmem = (BYTE *) 0xA0000000L;

void vga_plot(int x, int y, int color)
{
    vga_vmem[VGA_MEM_OFFSET(x, y)] = color;
}

void vga_wait_vblank(void)
{
    while ( (inp(VIDEO_STATUS_REGISTER) & 0x08))
        ;

    while (!(inp(VIDEO_STATUS_REGISTER) & 0x08))
        ;
}

void vga_set_color(BYTE index, BYTE red, BYTE green, BYTE blue)
{
    vga_wait_vblank();
    outp(VGA_DAC_PEL_ADDRESS, index);
    outp(VGA_DAC, red   >> 2);
    outp(VGA_DAC, green >> 2);
    outp(VGA_DAC, blue  >> 2);
}

void vga_clear_screen(void)
{
    vga_set_color(0, 0, 0, 0);
    memset(vga_vmem, 0, 320 * 200);
}

void vga_set_palette(struct color *palette)
{
    int i;

    vga_wait_vblank();
    outp(VGA_DAC_PEL_ADDRESS, 0);
    for (i = 0; i < 256; ++i) {
        outp(VGA_DAC, palette[i].red >> 2);
        outp(VGA_DAC, palette[i].green >> 2);
        outp(VGA_DAC, palette[i].blue >> 2);
    }
}

