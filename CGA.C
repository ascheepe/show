#include <string.h>

#include "system.h"
#include "cga.h"

static u8 far *vmem = (u8 far *) 0xB8000000L;

/*
 * CGA has 4 pixels per byte as such:
 * bit   : 7 6  5 4  3 2  1 0
 * color : 1 0  1 0  1 0  1 0
 *          \/   \/   \/   \/
 * pixel :  0    1    2    3
 *
 * even lines are stored at B8000
 * while odd lines are offset +2000;
 * BA000.
 */
void
cga_plot(u16 x, u16 y, u8 color)
{
	static u8 mask[] = { 0x3f, 0xcf, 0xf3, 0xfc };
	u8 y2 = y >> 1;
	u16 offset = (0x2000 * (y & 1)) + (y2 << 6) + (y2 << 4) + (x >> 2);
	u8 far *pixel = vmem + offset;
	u8 bitpos = x & 3;
	u8 val = *pixel;

	/* clear masked pixels */
	val &= mask[bitpos];

	/*
	 * set masked pixels:
	 *
	 * 0 ^ 3 = 3 => 3 * 2 = 6
	 * 1 ^ 3 = 2 => 2 * 2 = 4
	 * 2 ^ 3 = 1 => 1 * 2 = 2
	 * 3 ^ 3 = 0 => 0 * 2 = 0
	 *
	 */
	val |= (color & 3) << ((bitpos ^ 3) << 1);

	*pixel = val;
}

void
cga_clear_screen(void)
{
	memsetf(vmem, 0, 16U * 1024);
}
