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

#include <string.h>
#include <dos.h>

#include "system.h"
#include "mda.h"

static BYTE *mda_memory = (BYTE *) 0xB0000000L;

#define PORT_INDEX		0x3b4
#define PORT_DATA		(PORT_INDEX + 1)
#define PORT_CONTROL		0x3b8
#define PORT_CONFIG		0x3bf
#define CONFIG_HALF		1
#define MDA_MODE_GRAPHICS	2
#define MDA_MODE_TEXT		0x20
#define TABLE_SIZE		12

static BYTE mda_graphics_table[TABLE_SIZE] = {
	0x35, 0x2d, 0x2e, 0x07,
	0x5b, 0x02, 0x57, 0x57,
	0x02, 0x03, 0x00, 0x00,
};

static BYTE mda_text_table[TABLE_SIZE] = {
	0x61, 0x50, 0x52, 0x0f,
	0x19, 0x06, 0x19, 0x19,
	0x02, 0x0d, 0x0b, 0x0c,
};

void
mda_set_graphics_mode(int on)
{
	BYTE *table;
	int i;

	disable();

	/* half mode configuration */
	outp(PORT_CONFIG, CONFIG_HALF);

	/* change mode w/o screen on */
	if (on) {
		table = mda_graphics_table;
		outp(PORT_CONTROL, MDA_MODE_GRAPHICS);
	} else {
		table = mda_text_table;
		outp(PORT_CONTROL, MDA_MODE_TEXT);
	}

	/* setup 6845 */
	for (i = 0; i < TABLE_SIZE; ++i) {
		outp(PORT_INDEX, i);
		outp(PORT_DATA, table[i]);
	}

	/* set screen on, page 0 (2 = 0b10) */
	if (on)
		outp(PORT_CONTROL, MDA_MODE_GRAPHICS | 2);
	else
		outp(PORT_CONTROL, MDA_MODE_TEXT | 2);

	enable();
}

void
mda_plot(int x, int y, int color)
{
	BYTE *pixel = mda_memory + (0x2000 * (y & 3)) + (90 * (y >> 2))
	    + (x >> 3);
	BYTE value = 1 << (7 - (x & 7));

	if (color)
		*pixel |= value;
	else
		*pixel &= ~value;
}

void
mda_clear_screen(void)
{
	memset(mda_memory, 0, 32 * 1024);
}

