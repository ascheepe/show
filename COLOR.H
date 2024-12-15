#ifndef COLOR_H
#define COLOR_H

#include "system.h"

struct rgb {
	BYTE r, g, b;
};

enum {
	MAX_RANGE_RED,
	MAX_RANGE_GREEN,
	MAX_RANGE_BLUE
};

BYTE color_to_mono(struct rgb *);
#endif
