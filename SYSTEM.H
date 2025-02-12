#ifndef SYSTEM_H
#define SYSTEM_H
#include <stdio.h>
#include "globals.h"

void die(char *fmt, ...);
u8 read8(FILE *);
u16 read16(FILE *);
u32 read32(FILE *);
void setmode(int);
void far *memsetf(void far *, int, size_t);
int maybe_exit(void);
#endif
