#ifndef GLOBALS_H
#define GLOBALS_H

enum { false, true };

#define MODE_TXT 0x03
#define MODE_CGA 0x05
#define MODE_EGA 0x0d
#define MODE_VGA 0x13

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

#define KEY_ESC 27

#define MAX_IMAGE_WIDTH  320
#define MAX_IMAGE_HEIGHT 200
#define MAX_IMAGE_COLORS 256

struct rgb {
	u8 r, g,b;
};

extern int graphics_mode;

extern u16 x_offset;
extern u16 y_offset;

extern u8 image_row[MAX_IMAGE_WIDTH];
extern struct rgb image_palette[MAX_IMAGE_COLORS];
extern u16 image_width;
extern u16 image_height;

extern u8 mda_palette[2];
extern u8 cga_palette[4];
extern struct rgb std_palette[16];
extern void (*plot)(u16, u16, u8);
#endif
