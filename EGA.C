#include <conio.h>
#include <dos.h>
#include <string.h>

#include "system.h"
#include "ega.h"
#include "color.h"

static BYTE far *vmem = (BYTE far *) 0xA0000000L;

/*
 * EGA stores colors as
 * 7 6 5 4 3 2 1 0
 * | | | | | | | +- Blue  MSB
 * | | | | | | +--- Green MSB
 * | | | | | +----- Red   MSB
 * | | | | +------- Blue  LSB
 * | | | +--------- Green LSB
 * | | +----------- Red   LSB
 * | +------------- Reserved
 * +--------------- Reserved
 */
static BYTE
ega_make_color(struct rgb *color)
{
	BYTE r = color->r >> 6;
	BYTE g = color->g >> 6;
	BYTE b = color->b >> 6;

	BYTE r_msb = r >> 1;
	BYTE g_msb = g >> 1;
	BYTE b_msb = b >> 1;

	BYTE r_lsb = r & 1;
	BYTE g_lsb = g & 1;
	BYTE b_lsb = b & 1;

	return (b_msb << 0) | (g_msb << 1) | (r_msb << 2)
	    | (b_lsb << 3) | (g_lsb << 4) | (r_lsb << 5);
}

/*
 * Attribute registers are accessed via I/O port 0x3c0.
 * This register can act like a flip-flop to automatically
 * switch between setting the palette index and the palette
 * value. To enable this mode a read of port 0x3da is
 * required. The datasheet calls this 'sending an IOR command'.
 *
 * After that the first write selects the attribute
 * and the second sets the value.
 *
 * Attributes 0x00 - 0x0F specify the 16 color palette in
 * the format as the make_color function provides.
 */
void
ega_set_palette(struct rgb *palette, int ncolors)
{
	int i;

	/* enable 0x3c0 flip-flop */
	inp(0x3da);

	/* and set the palette */
	for (i = 0; i < ncolors; ++i) {
		outp(0x3c0, i);
		outp(0x3c0, ega_make_color(&palette[i]));
	}
}

void
ega_plot(int x, int y, int color)
{
	BYTE far *pixel = vmem + (y << 6) + (y << 4) + (x >> 3);
	BYTE mask = 0x80 >> (x & 7);

	/*
	 * color selects which planes to write to
	 * this is the palette index value
	 * e.g. 11 is cyan with default palette.
	 */
	outp(0x3c4, 2);
	outp(0x3c5, color);

	/* set pixel mask */
	outp(0x3ce, 8);
	outp(0x3cf, mask);

	/*
	 * with the mask set above we can just
	 * write all 1's
	 */
	*pixel |= 0xff;
}

void
ega_clear_screen(void)
{
	setmode(MODE_EGA);
#if 0
	int x, y;

	/* Write rgbi to all planes in parallel */
	outp(0x3c4, 2);
	outp(0x3c5, 0xf);

	for (y = 0; y < EGA_HEIGHT; ++y) {
		BYTE far *offset = vmem + (y << 6) + (y << 4);

		for (x = 0; x < EGA_WIDTH / 8; ++x) {
			BYTE far *pixel = offset + x;

			*pixel = 0x00;
		}
	}
#endif
}
