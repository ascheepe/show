#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>

#include "system.h"

/*
 * Return to text mode, print a message and
 * exit with error.
 */
void
die(char *fmt, ...)
{
	va_list vp;

	setmode(MODE_TEXT);

	va_start(vp, fmt);
	vfprintf(stderr, fmt, vp);
	va_end(vp);

	if (fmt[0] && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s", strerror(errno));

	exit(1);
}

/*
 * Allocate memory with error checking.
 */
void *
xmalloc(size_t size)
{
	void *ptr;

	ptr = malloc(size);
	if (ptr == NULL)
		die("malloc: out of memory.");

	return ptr;
}

/*
 * Allocate and clear memory with error checking.
 */
void *
xcalloc(size_t nmemb, size_t size)
{
	void *ptr;

	ptr = calloc(nmemb, size);
	if (ptr == NULL)
		die("calloc: out of memory.");

	return ptr;
}

/*
 * Reallocate memory with error checking.
 */
void *
xrealloc(void *ptr, size_t size)
{
	void *new_ptr;

	new_ptr = realloc(ptr, size);
	if (new_ptr == NULL)
		die("realloc: out of memory");

	return new_ptr;
}

/*
 * Read a byte with error checking.
 */
BYTE
read_byte(FILE *fp)
{
	int ch;

	if ((ch = fgetc(fp)) == EOF)
		die("read_byte: input error.");

	return ch;
}

/*
 * Read a word with error checking.
 */
WORD
read_word(FILE *fp)
{
	BYTE buf[2];

	if (fread(buf, 2, 1, fp) != 1)
		die("read_word: input error.");

	return buf[0] | (buf[1] << 8);
}

/*
 * Read a double word with error checking.
 */
DWORD
read_dword(FILE *fp)
{
	BYTE buf[4];

	if (fread(buf, 4, 1, fp) != 1)
		die("read_dword: input error.");

	return (DWORD) buf[0] | ((DWORD) buf[1] << 8) |
	    ((DWORD) buf[2] << 16) | ((DWORD) buf[3] << 24);
}

/*
 * Set mode via bios call.
 */
void
setmode(int mode)
{
	union REGS regs;

	regs.h.ah = 0;
	regs.h.al = mode;
	int86(0x10, &regs, &regs);
}

/*
 * Test if a file exists.
 */
int
file_exists(char *filename)
{
	FILE *fp;

	fp = fopen(filename, "rb");
	if (fp == NULL)
		return false;

	fclose(fp);
	return true;
}

/*
 * memset for far pointers
 */
void far *memsetf(void far *s, int c, size_t n)
{
	char far *p = s;

	while (n-- > 0)
		*p++ = c;

	return s;
}

/*
 * memcpy for far pointers
 */
void far *memcpyf(void far *dest, void far *src, size_t n)
{
	char far *pdest = dest;
	char far *psrc = src;

	while (n-- > 0)
		*pdest++ = *psrc++;

	return dest;
}

