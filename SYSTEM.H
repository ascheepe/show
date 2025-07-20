#ifndef SYSTEM_H
#define SYSTEM_H
#include <stdio.h>
#include "globals.h"

void die(char *fmt, ...);
BYTE read_byte(FILE *);
WORD read_word(FILE *);
DWORD read_dword(FILE *);
void setmode(int);
void far *memsetf(void far *, int, size_t);
int maybe_exit(void);
#endif
