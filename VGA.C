#include <conio.h>
#include <dos.h>

#include "globals.h"
#include "system.h"
#include "vga.h"

#define VGA_DAC_PEL_ADDRESS 0x3c8
#define VGA_DAC 0x3c9
#define VIDEO_STATUS_REGISTER 0x3da

#define INDEX(x, y) (((y) << 8) + ((y) << 6) + (x))
static BYTE far *vmem = (BYTE far *) 0xA0000000L;

void
vga_plot(WORD x, WORD y, BYTE color)
{
	vmem[INDEX(x, y)] = color;
}

void
vga_plot_row(BYTE y, BYTE *rowdata)
{
	WORD x, offset;

	offset = INDEX(0, y); /* + x_offset; */
	for (x = 0; x < MAX_IMAGE_WIDTH; ++x)
		vmem[offset + x] = rowdata[x];
}

void
vga_wait_vblank(void)
{
	while ((inp(VIDEO_STATUS_REGISTER) & 0x08))
		continue;

	while (!(inp(VIDEO_STATUS_REGISTER) & 0x08))
		continue;
}

void
vga_set_color(BYTE index, BYTE r, BYTE g, BYTE b)
{
	vga_wait_vblank();
	outp(VGA_DAC_PEL_ADDRESS, index);
	outp(VGA_DAC, r >> 2);
	outp(VGA_DAC, g >> 2);
	outp(VGA_DAC, b >> 2);
}

void
vga_clear_screen(void)
{
	vga_set_color(0, 0, 0, 0);
	memsetf(vmem, 0, 320U * 200);
}

void
vga_set_palette(struct rgb *palette)
{
	int i;

	vga_wait_vblank();
	outp(VGA_DAC_PEL_ADDRESS, 0);
	for (i = 0; i < 256; ++i) {
		outp(VGA_DAC, palette[i].r >> 2);
		outp(VGA_DAC, palette[i].g >> 2);
		outp(VGA_DAC, palette[i].b >> 2);
	}
}
