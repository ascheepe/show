#include <dos.h>
#include <string.h>

#include "globals.h"
#include "cplus.h"
#include "system.h"

static BYTE far *vmem = (BYTE far *) 0xB8000000L;
static BYTE pixelmask[] = { 0x3f, 0xcf, 0xf3, 0xfc };

/*
 * Colorplus works like cga, except the color bits
 * are stored at B8000 (+2000) for red/green and
 * BC000 (+2000) for blue/intensity.
 */
void
cplus_plot(int x, int y, int palidx)
{
	BYTE y2 = y >> 1;
	WORD offset = (0x2000 * (y & 1)) + (y2 << 6) + (y2 << 4) + (x >> 2);
	BYTE far *rgpixel = vmem + offset;
	BYTE far *bipixel = vmem + offset + 0x4000;
	BYTE bitpos = x & 3;
	BYTE rgval = *rgpixel;
	BYTE bival = *bipixel;
	BYTE r, g, b, i;

	if (palidx < 8) {
		r = std_palette[palidx].r ? 2 : 0;
		g = std_palette[palidx].g ? 1 : 0;
		b = std_palette[palidx].b ? 2 : 0;
		i = 0;
	} else {
		r = std_palette[palidx].r == 0xff ? 2 : 0;
		g = std_palette[palidx].g == 0xff ? 1 : 0;
		b = std_palette[palidx].b == 0xff ? 2 : 0;
		i = 1;
	}

	rgval &= pixelmask[bitpos];
	rgval |= (r | g) << ((bitpos ^ 3) << 1);
	bival &= pixelmask[bitpos];
	bival |= (b | i) << ((bitpos ^ 3) << 1);

	*rgpixel = rgval;
	*bipixel = bival;
}

void
cplus_init(void)
{
	outp(0x3dd, 1 << 4);
}

void
cplus_clear_screen(void)
{
	memsetf(vmem, 0, 32 * 1024);
}
