#ifndef VGA_H
#define VGA_H

#include "globals.h"
#include "system.h"

#define VGA_WIDTH 320
#define VGA_HEIGHT 200

void vga_plot(u16, u16, u8);
void vga_plot_row(u8, u8 *);
void vga_clear_screen(void);
void vga_set_color(u8, u8, u8, u8);
void vga_set_palette(struct rgb *);
void vga_wait_vblank(void);

#endif
