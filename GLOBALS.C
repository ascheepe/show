#include "system.h"

/* Offsets to center the picture */
WORD x_offset;
WORD y_offset;

void (*plot)(int x, int y, int color);
