#include <dos.h>
#include <string.h>

#include "globals.h"
#include "cplus.h"
#include "system.h"

static BYTE far *vmem = (BYTE far *) 0xB8000000L;

/*
 * Colorplus has 4 pixels per byte as such:
 * bit   : 7 6  5 4  3 2  1 0
 * color : 1 0  1 0  1 0  1 0
 *          \/   \/   \/   \/
 * pixel :  0    1    2    3
 *
 * even lines are stored at B8000
 * while odd lines are offset +2000;
 * BA000.
 *
 * The red/green bits are stored as above while the
 * blue/intensity bits are stored at BC000.
 */
void
cplus_plot(int x, int y, int palidx)
{
	BYTE mask[] = { 0x3f, 0xcf, 0xf3, 0xfc };
	BYTE far *rgpixel = vmem + (0x2000 * (y & 1))
	+ (80 * (y >> 1)) + (x >> 2);
	BYTE far *bipixel = vmem + 0x4000 + (0x2000 * (y & 1))
	+ (80 * (y >> 1)) + (x >> 2);
	BYTE bitpos = x & 3;
	BYTE rgval = *rgpixel;
	BYTE bival = *bipixel;
	BYTE r, g, b, i, rg, bi;

	if (palidx < 8) {
		r = cplus_palette[palidx].r ? 2 : 0;
		g = cplus_palette[palidx].g ? 1 : 0;
		b = cplus_palette[palidx].b ? 2 : 0;
		i = 0;
	} else {
		r = cplus_palette[palidx].r == 0xff ? 2 : 0;
		g = cplus_palette[palidx].g == 0xff ? 1 : 0;
		b = cplus_palette[palidx].b == 0xff ? 2 : 0;
		i = 1;
	}

	rg = r | g;
	bi = b | i;

	rgval &= mask[bitpos];
	rgval |= rg << ((bitpos ^ 3) << 1);
	bival &= mask[bitpos];
	bival |= bi << ((bitpos ^ 3) << 1);

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