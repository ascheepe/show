#include <dos.h>

#include "detect.h"

/*
 * Try to detect the used graphics card;
 * MDA, CGA, EGA or VGA.
 * XXX: instead of returning CGA if all else
 * fails try better detection of CGA instead
 * and default to MDA instead.
 */
enum graphics_type
detect_graphics(void)
{
	union REGS regs;

	/* First try int10 service */
	regs.h.ah = 0x1a;
	regs.h.al = 0x00;
	int86(0x10, &regs, &regs);

	if (regs.h.al == 0x1a) {
		switch (regs.h.bl) {
		case 0x00:
			return GRAPHICS_ERROR;
		case 0x01:
			return MDA_GRAPHICS;
		case 0x02:
			return CGA_GRAPHICS;
		case 0x04:
		case 0x05:
			return EGA_GRAPHICS;
		case 0x07:
		case 0x08:
			return VGA_GRAPHICS;
		}

		/* if we had int10 service assume CGA */
		return CGA_GRAPHICS;
	}

	/* If not detected yet check if it's an EGA card */
	regs.h.ah = 0x12;
	regs.x.bx = 0x10;
	int86(0x10, &regs, &regs);

	if (regs.x.bx != 0x10)
		return EGA_GRAPHICS;

	/* Try equipment info if still not found */
	int86(0x11, &regs, &regs);

	if (((regs.h.al & 0x30) >> 4) == 3)
		return MDA_GRAPHICS;

	/* if all else fails assume CGA */
	return CGA_GRAPHICS;
}

int
is_cplus(void)
{
	char far *p = (char far *) 0xB8000000L;
	char far *q = (char far *) 0xBC000000L;

	/* try to enable colorplus */
	outp(0x3dd, 1 << 4);

	/* plain cga maps BC00 to B800 */
	*q = 0xaa;
	*p = 0x55;
	if (*q == 0x55)
		return 0;

	/* colorplus can swap pages */
	outp(0x3dd, (1 << 6) | (1 << 4));
	if (*p == 0xaa && *q == 0x55) {
		return 1;
	}

	return 0;
}
