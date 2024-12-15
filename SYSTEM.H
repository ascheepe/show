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

void die(char *fmt, ...);
void *xmalloc(size_t);
void *xcalloc(size_t, size_t);
void *xrealloc(void *, size_t);
BYTE read_byte(FILE *);
WORD read_word(FILE *);
DWORD read_dword(FILE *);
void setmode(int);
int file_exists(char *);
void far *memsetf(void far *, int, size_t);
void far *memcpyf(void far *, void far *, size_t);
extern int maybe_exit(void);
#endif
