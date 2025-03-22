#include <dos.h>

#include "detect.h"
#include "system.h"

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

	/* VGA */
	regs.x.ax = 0x1200;
	regs.h.bl = 0x32;
	int86(0x10, &regs, &regs);
	if (regs.h.al == 0x12)
		return VGA_GRAPHICS;

	/* EGA */
	regs.h.ah = 0x12;
	regs.x.bx = 0xff10;
	int86(0x10, &regs, &regs);
	if (regs.h.bh != 0xff)
		return EGA_GRAPHICS;

	/* MDA */
	regs.h.ah = 0x0f;
	int86(0x10, &regs, &regs);
	if (regs.h.al == 0x07)
		return MDA_GRAPHICS;

	/* PcJr */
	if (*((u8 far *)(0xffff000eL)) == 0xfd)
		return TGA_GRAPHICS;

	/* Tandy */
	if (*((u8 far *)(0xffff000eL)) == 0xff
	&& *((u8 far *)(0xfc000000L)) == 0x21)
		return TGA_GRAPHICS;

	/* If all failed it must be CGA */
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
	if (*p == 0xaa && *q == 0x55)
		return 1;

	return 0;
}