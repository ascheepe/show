#ifndef DETECT_H
#define DETECT_H

enum graphics_type {
	GRAPHICS_ERROR,
	MDA_GRAPHICS,
	CGA_GRAPHICS,
	EGA_GRAPHICS,
	VGA_GRAPHICS
};

enum graphics_type detect_graphics(void);

#endif
