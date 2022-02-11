/*
 * Copyright (c) 2020 Axel Scheepers
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

#include "system.h"

int show_progress = false;

void
xerror(char *message)
{
	set_mode(MODE_TEXT);
	fprintf(stderr, "%s\n", message);
	exit(1);
}


void *
xmalloc(size_t size)
{
	void *ret;

	if ((ret = malloc(size)) == NULL)
		xerror("malloc: no more memory.");

	return ret;
}

void *
xcalloc(size_t nmemb, size_t size)
{
	void *ret;

	if ((ret = calloc(nmemb, size)) == NULL)
		xerror("xcalloc: no more memory.");

	return ret;
}

void *
xrealloc(void *ptr, size_t size)
{
	void *ret;

	if ((ret = realloc(ptr, size)) == NULL)
		xerror("xrealloc: no more memory.");

	return ret;
}

BYTE
read_byte(FILE *f)
{
	int ch;

	if ((ch = fgetc(f)) == EOF)
		xerror("read_byte: I/O Error");

	return ch;
}

WORD
read_word(FILE *f)
{
	BYTE bytes[2];

	if (fread(bytes, 2, 1, f) != 1)
		xerror("read_word: I/O Error");

	return (bytes[0] << 0) | (bytes[1] << 8);
}

DWORD
read_dword(FILE *f)
{
	BYTE bytes[4];

	if (fread(bytes, 4, 1, f) != 1)
		xerror("read_dword: I/O Error");

	return (bytes[0] << 0) | (bytes[1] << 8) | (bytes[1] << 16) |
	    (bytes[2] << 24);
}

void
set_mode(int mode)
{
	union REGS regs = { 0 };

	regs.h.ah = 0;
	regs.h.al = mode;
	int86(0x10, &regs, &regs);
}

int
file_exists(char *filename)
{
	FILE *test = fopen(filename, "rb");

	if (test == NULL)
		return false;

	fclose(test);
	return true;
}
