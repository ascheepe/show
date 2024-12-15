#ifndef GLOBALS_H
#define GLOBALS_H

#include "color.h"
#include "system.h"

#define MAX_IMAGE_WIDTH  320
#define MAX_IMAGE_HEIGHT 200
#define MAX_IMAGE_COLORS 256

extern int graphics_mode;

extern WORD x_offset;
extern WORD y_offset;

extern BYTE image_row[MAX_IMAGE_WIDTH];
extern struct rgb image_palette[MAX_IMAGE_COLORS];
extern int image_width;
extern int image_height;

extern BYTE mda_palette[2];
extern BYTE cga_palette[4];
extern struct rgb ega_palette[16];
extern void (*plot)(int, int, int);
#endif
