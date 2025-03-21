#ifndef DETECT_H
#define DETECT_H

enum graphics_type {
	GRAPHICS_ERROR = -1,
	MDA_GRAPHICS,
	CGA_GRAPHICS,
	CPLUS_GRAPHICS,
	TGA_GRAPHICS,
	EGA_GRAPHICS,
	VGA_GRAPHICS
};

enum graphics_type detect_graphics(void);
int is_cplus(void);
int is_tga(void);
#endif
