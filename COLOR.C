#include "color.h"

/*
 * Convert a color to grayscale
 */
BYTE
color_to_mono(struct rgb *color)
{
	return
		color->r * 30 / 100 +
		color->g * 59 / 100 +
		color->b * 11 / 100;
}
