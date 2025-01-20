#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <errno.h>

#include "system.h"

/*
 * Return to text mode, print a message and
 * exit with error.
 */
void
die(char *fmt, ...)
{
	va_list vp;

	setmode(MODE_TXT);

	va_start(vp, fmt);
	vfprintf(stderr, fmt, vp);
	va_end(vp);

	exit(1);
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
 * memset for far pointers
 */
void far *
memsetf(void far *s, int c, size_t n)
{
	WORD cc = (c << 8) | c;
	WORD far *wp = (WORD far *)s;
	size_t nw = n / sizeof(WORD);
	char far *p;

	n -= nw * sizeof(WORD);
	while (nw-- > 0)
		*wp++ = cc;

	p = (char far *)wp;
	while (n-- > 0)
		*p++ = c;

	return s;
}

int
maybe_exit(void)
{
	int ch;

	if (!kbhit())
		return 0;

	ch = getch();

	/* read away function/arrow keys */
	if (ch == 0 || ch == 224)
		return getch();

	if (ch == KEY_ESC || tolower(ch) == 'q') {
		setmode(MODE_TXT);
		exit(0);
	}

	return ch;
}
