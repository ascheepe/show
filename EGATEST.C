#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>

typedef unsigned char BYTE;
typedef unsigned int  WORD;
typedef unsigned long DWORD;

enum { false, true };

struct color {
    BYTE red;
    BYTE green;
    BYTE blue;
};

void set_mode(int mode);

void xerror(char *message)
{
    set_mode(0x03);
    fprintf(stderr, "%s\n", message);
    exit(1);
}


void *xmalloc(size_t size)
{
    void *ptr;

    ptr = malloc(size);
    if (ptr == NULL) {
        xerror("malloc: no more memory.");
    }

    return ptr;
}

void *xcalloc(size_t nmemb, size_t size)
{
    void *ptr;

    ptr = calloc(nmemb, size);
    if (ptr == NULL) {
        xerror("xcalloc: no more memory.");
    }

    return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
    void *new_ptr;

    new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) {
        xerror("xrealloc: no more memory.");
    }

    return new_ptr;
}

void set_mode(int mode)
{
    union REGS regs = { 0 };

    regs.h.ah = 0;
    regs.h.al = mode;
    int86(0x10, &regs, &regs);
}

static BYTE *vmem = (BYTE *) 0xA0000000L;

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
BYTE ega_make_color(struct color *color)
{
    BYTE red   = color->red   >> 6;
    BYTE green = color->green >> 6;
    BYTE blue  = color->blue  >> 6;

    BYTE red_msb   = red   >> 1;
    BYTE green_msb = green >> 1;
    BYTE blue_msb  = blue  >> 1;

    BYTE red_lsb   = red   & 1;
    BYTE green_lsb = green & 1;
    BYTE blue_lsb  = blue  & 1;

    return (blue_msb << 0) | (green_msb << 1) | (red_msb << 2) |
           (blue_lsb << 3) | (green_lsb << 4) | (red_lsb << 5);
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
void ega_set_palette(struct color *palette, int ncolors)
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

void ega_plot(int x, int y, int color)
{
    BYTE *pixel = vmem + (y << 5) + (y << 3) + (x >> 3);
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

void ega_hi_plot(int x, int y, int color)
{
    BYTE *pixel = vmem + (y << 6) + (y << 4) + (x >> 3);
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

void ega_clear_screen(void)
{
    set_mode(0x10);
    /* memset(vmem, 0, 128 * 1024); */
}

static void ega_hi_show(void)
{
    struct color palette[] = {
        { 0x00, 0x00, 0x00 },
        { 0x55, 0x55, 0x55 },
        { 0xAA, 0xAA, 0xAA },
        { 0xFF, 0xFF, 0xFF },
        { 0x55, 0x00, 0x00 },
        { 0xAA, 0x00, 0x00 },
        { 0xFF, 0x00, 0x00 },
        { 0x00, 0x55, 0x00 },
        { 0x00, 0xAA, 0x00 },
        { 0x00, 0xFF, 0x00 },
        { 0x00, 0x00, 0x55 },
        { 0x00, 0x00, 0xAA },
        { 0xAA, 0x55, 0x00 },
        { 0xFF, 0xAA, 0x00 },
        { 0x55, 0xFF, 0xFF },
        { 0xFF, 0x55, 0xFF }
    };
    BYTE color;
    int row, col;

    set_mode(0x10);
    ega_set_palette(palette, 16);

    color = 0;
    for (row = 0; row < 350; ++row) {
        if ((row > 0) && (row % (350/16) == 0)) {
            ++color;
        }
        for (col = 0; col < 640; ++col) {
            ega_hi_plot(col, row, color);
        }
    }
}

#define KEY_ESC 27
static int quit(void)
{
    if (kbhit()) {
        switch (getch()) {
            case 'q':
            case 'Q':
            case KEY_ESC:
                return 1;

            /* read away special key */
            case 0:
            case 224:
                getch();
                break;
        }
    }

    return 0;
}

int main(void)
{
    ega_hi_show();

    while (!quit()) {
        /* wait */
    }

    set_mode(0x03);
    return 0;
}

