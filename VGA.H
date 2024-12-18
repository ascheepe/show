#ifndef VGA_H
#define VGA_H

#include "color.h"
#include "system.h"

#define VGA_WIDTH 320
#define VGA_HEIGHT 200

void vga_plot(int, int, int);
void vga_plot_row(int, BYTE *);
void vga_clear_screen(void);
void vga_set_color(BYTE, BYTE, BYTE, BYTE);
void vga_set_palette(struct rgb *);
void vga_wait_vblank(void);

#endif
