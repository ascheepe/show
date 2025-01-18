#ifndef SYSTEM_H
#define SYSTEM_H
#include <stdio.h>

enum { false, true };

/* int10 modes */
#define MODE_TEXT 0x03
#define MODE_CGA 0x05
#define MODE_EGA 0x10
#define MODE_VGA 0x13

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#define KEY_ESC 27

void die(char *fmt, ...);
BYTE read_byte(FILE *);
WORD read_word(FILE *);
DWORD read_dword(FILE *);
void setmode(int);
void far *memsetf(void far *, int, size_t);
int file_exists(char *);
int maybe_exit(void);
#endif
