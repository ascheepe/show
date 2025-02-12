#ifndef SYSTEM_H
#define SYSTEM_H
#include <stdio.h>
#include "globals.h"

void die(char *fmt, ...);
u8 read_byte(FILE *);
u16 read_word(FILE *);
u32 read_dword(FILE *);
void setmode(int);
void far *memsetf(void far *, int, size_t);
int maybe_exit(void);
#endif
