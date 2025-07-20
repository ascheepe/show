#include <dos.h>

#include "bios.h"

void
bios_plot(WORD x, WORD y, BYTE color)
{
	union REGS regs;

	regs.h.ah = 0x0c;  /* set pixel */
	regs.h.al = color;
	regs.h.bh = 0x00;  /* page */
	regs.x.cx = x;
	regs.x.dx = y;
	int86(0x10, &regs, &regs);
}

void
bios_clear_screen(void)
{
	WORD x, y;

	for (y = 0; y < BIOS_HEIGHT; ++y) {
		for (x = 0; x < BIOS_WIDTH; ++x)
			bios_plot(x, y, 0);
	}
}
