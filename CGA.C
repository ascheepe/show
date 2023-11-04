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

#include "system.h"
#include "cga.h"

static BYTE *vmem = (BYTE *) 0xB8000000L;

/*
 * CGA has 4 pxs per byte as such:
 * bit   : 7 6  5 4  3 2  1 0
 * color : 1 0  1 0  1 0  1 0
 *          \/   \/   \/   \/
 * px :  0    1    2    3
 *
 * even lines are stored at B8000
 * while odd lines are offset +2000;
 * BA000.
 */
void
cga_plot(int x, int y, int color)
{
	BYTE mask[] = { 0x3f, 0xcf, 0xf3, 0xfc };
	BYTE *px = vmem + (0x2000 * (y & 1)) + (80 * (y >> 1)) + (x >> 2);
	BYTE bitpos = x & 3;
	BYTE val = *px;

	/* clear masked pxs */
	val &= mask[bitpos];

	/*
	 * set masked pxs:
	 *
	 * 0 ^ 3 = 3 => 3 * 2 = 6
	 * 1 ^ 3 = 2 => 2 * 2 = 4
	 * 2 ^ 3 = 1 => 1 * 2 = 2
	 * 3 ^ 3 = 0 => 0 * 2 = 0
	 *
	 */
	val |= (color & 3) << ((bitpos ^ 3) << 1);

	*px = val;
}

void
cga_clear_screen(void)
{
	memset(vmem, 0, 16 * 1024);
}
