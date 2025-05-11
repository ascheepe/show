#include <dos.h>

#include "bios.h"
#include "tga.h"

void
tga_plot(u16 x, u16 y, u8 color)
{
	bios_plot(x, y, color);
}

void
tga_clear_screen(void)
{
	bios_clear_screen();
}
