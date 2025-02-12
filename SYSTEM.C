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
u8
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
u16
read_word(FILE *fp)
{
	u8 buf[2];

	if (fread(buf, 2, 1, fp) != 1)
		die("read_word: input error.");

	return buf[0] | (buf[1] << 8);
}

/*
 * Read a double word with error checking.
 */
u32
read_dword(FILE *fp)
{
	u8 buf[4];

	if (fread(buf, 4, 1, fp) != 1)
		die("read_dword: input error.");

	return (u32) buf[0] | ((u32) buf[1] << 8) |
	    ((u32) buf[2] << 16) | ((u32) buf[3] << 24);
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
	u16 cc = (c << 8) | c;
	u16 far *wp = (u16 far *)s;
	size_t nw = n / sizeof(u16);
	char far *p;

	n -= nw * sizeof(u16);
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