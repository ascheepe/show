#include <dos.h>

#include "bios.h"
#include "tga.h"

void
tga_plot(WORD x, WORD y, BYTE color)
{
	bios_plot(x, y, color);
}

void
tga_clear_screen(void)
{
	bios_clear_screen();
}
