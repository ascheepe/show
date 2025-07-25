#include <conio.h>
#include <dos.h>
#include <string.h>

#include "ega.h"
#include "globals.h"
#include "system.h"

static BYTE far *vmem = (BYTE far *) 0xA0000000L;

void
ega_plot(WORD x, WORD y, BYTE color)
{
	BYTE far *pixel = vmem + (y << 5) + (y << 3) + (x >> 3);
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

void
ega_clear_screen(void)
{
	setmode(MODE_EGA);
	/* memsetf(vmem, 0, 64000U); */
}
