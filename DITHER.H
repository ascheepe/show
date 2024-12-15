#ifndef DITHER_H
#define DITHER_H

#include "bitmap.h"
#include "color.h"

void grayscale_dither(int, BYTE *, int);
void color_dither(int, struct rgb *, int);
void show(int);
void dither_init(void);

#endif
