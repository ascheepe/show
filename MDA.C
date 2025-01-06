#include <conio.h>
#include <dos.h>
#include <string.h>

#include "system.h"
#include "mda.h"

static BYTE far *vmem = (BYTE far *) 0xB0000000L;

#define PORT_INDEX 0x3b4
#define PORT_DATA (PORT_INDEX + 1)
#define PORT_CONTROL 0x3b8
#define PORT_CONFIG 0x3bf

#define CONFIG_HALF 1

#define MDA_MODE_GRAPHICS 2
#define MDA_MODE_TEXT 0x20

static BYTE graphics_init[] = {
	0x35, 0x2d, 0x2e, 0x07,
	0x5b, 0x02, 0x57, 0x57,
	0x02, 0x03, 0x00, 0x00,
};

static BYTE text_init[] = {
	0x61, 0x50, 0x52, 0x0f,
	0x19, 0x06, 0x19, 0x19,
	0x02, 0x0d, 0x0b, 0x0c,
};

void
mda_set_mode(int mode)
{
	BYTE *data;
	size_t len;
	int i;

	/* half mode configuration */
	outp(PORT_CONFIG, CONFIG_HALF);

	/* change mode w/o screen on */
	if (mode == MDA_GRAPHICS_MODE) {
		data = graphics_init;
		len = sizeof(graphics_init);
		outp(PORT_CONTROL, MDA_MODE_GRAPHICS);
	} else {
		data = text_init;
		len = sizeof(text_init);
		outp(PORT_CONTROL, MDA_MODE_TEXT);
	}

	/* setup 6845 */
	for (i = 0; i < len; ++i) {
		outp(PORT_INDEX, i);
		outp(PORT_DATA, data[i]);
	}

	/* set screen on, page 0 (2 = 0b10) */
	if (mode == MDA_GRAPHICS_MODE)
		outp(PORT_CONTROL, MDA_MODE_GRAPHICS | 2);
	else
		outp(PORT_CONTROL, MDA_MODE_TEXT | 2);
}

void
mda_plot(int x, int y, int color)
{
	BYTE far *pixel =
	    vmem + (0x2000 * (y & 3)) + (90 * (y >> 2)) + (x >> 3);
	BYTE val = 1 << (7 - (x & 7));

	if (color)
		*pixel |= val;
	else
		*pixel &= ~val;
}

void
mda_clear_screen(void)
{
	memsetf(vmem, 0, 32 * 1024);
}
