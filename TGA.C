#include <dos.h>

#include "system.h"
#include "tga.h"

static u8 far *vmem = (u8 far *) 0xB8000000L;

/*
 * Tandy's screen is divided into 4 banks of 8kb,
 * scanlines are interleaved in groups of 4:
 *
 * bank 0 stores lines 0, 4, 8, 12, ...
 * bank 1 stores lines 1, 5, 9, 13, ...
 * bank 2 stores lines 2, 6, 10, 14, ...
 * bank 3 stores lines 3, 7, 11, 15, ...
 *
 * Each bank has 50 scanlines stored sequentially.
 * Each byte stores 2 pixels in each nibble with
 * color values from 0 to 15.
 */
void
tga_plot(u16 x, u16 y, u8 color)
{
	u8 far *pixel = vmem + 0x2000L * (y & 3) + ((y >> 2) * 160) + (x >> 1);

	if (x & 1)
		*pixel = (*pixel & 0xf0) | (color & 0x0f);
	else
		*pixel = (*pixel & 0x0f) | ((color & 0x0f) << 4);
}

void
tga_clear_screen(void)
{
	memsetf(vmem, 0, 32U * 1024);
}
