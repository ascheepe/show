#ifndef GLOBALS_H
#define GLOBALS_H

enum { false, true };

#define MODE_TXT 0x03
#define MODE_CGA 0x05
#define MODE_EGA 0x0d
#define MODE_VGA 0x13

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#define KEY_ESC 27

#define MAX_IMAGE_WIDTH  320
#define MAX_IMAGE_HEIGHT 200
#define MAX_IMAGE_COLORS 256

struct rgb {
	BYTE r, g,b;
};

extern int graphics_mode;

extern WORD x_offset;
extern WORD y_offset;

extern BYTE image_row[MAX_IMAGE_WIDTH];
extern struct rgb image_palette[MAX_IMAGE_COLORS];
extern int image_width;
extern int image_height;

extern BYTE mda_palette[2];
extern BYTE cga_palette[4];
extern struct rgb std_palette[16];
extern void (*plot)(int, int, int);
#endif
