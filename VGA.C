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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dos.h>
#include <conio.h>

#include "system.h"
#include "vga.h"

BYTE *vga_memory = (BYTE *) 0xA0000000L;

void
vga_plot(int x, int y, int color)
{
	vga_memory[VGA_MEM_OFFSET(x, y)] = color;
}

void
vga_wait_vblank(void)
{
	while ((inp(0x03da) & 0x08))
		;

	while (!(inp(0x03da) & 0x08))
		;
}

void
vga_set_color(BYTE index, BYTE r, BYTE g, BYTE b)
{
	vga_wait_vblank();
	outp(0x03C8, index);
	outp(0x03C9, r >> 2);
	outp(0x03C9, g >> 2);
	outp(0x03C9, b >> 2);
}

void
vga_clear_screen(void)
{
	vga_set_color(0, 0, 0, 0);
	memset(vga_memory, 0, 320 * 200);
}

void
vga_set_palette(BYTE * palette)
{
	int i;

	vga_wait_vblank();
	outp(0x03C8, 0);
	for (i = 0; i < 256 * 3; ++i)
		outp(0x03C9, palette[i] >> 2);
}


