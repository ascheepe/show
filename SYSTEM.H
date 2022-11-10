/*
 * Copyright (c) 2020-2022 Axel Scheepers
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

#ifndef SYSTEM_H
#define SYSTEM_H
#include <stdio.h>

enum { false, true };

/* int10 modes */
#define MODE_TEXT  0x03
#define MODE_CGA   0x04
#define MODE_CGA2  0x05
#define MODE_EGA   0x0D
#define MODE_EGAHI 0x10
#define MODE_VGA   0x13

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;

void xerror(char *);
void *xmalloc(size_t);
void *xcalloc(size_t, size_t);
void *xrealloc(void *, size_t);
BYTE read_byte(FILE *);
WORD read_word(FILE *);
DWORD read_dword(FILE *);
void set_mode(int);
int file_exists(char *);

#endif

