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

#include <stdlib.h>
#include "system.h"
#include "array.h"

struct array *
array_new(void)
{
	struct array *a;

#define INITIAL_CAPACITY 16
	a = xmalloc(sizeof(struct array));
	a->items = xmalloc(INITIAL_CAPACITY * sizeof(void *));
	a->size = 0;
	a->cap = INITIAL_CAPACITY;

	return a;
}

void
array_free(struct array *a)
{
	free(a->items);
	free(a);
}

void
array_add(struct array *a, void *data)
{
	if (a->size == a->cap) {
		size_t newcap = a->cap * 2;

		a->items = xrealloc(a->items, sizeof(void *) * newcap);
		a->cap = newcap;
	}

	a->items[a->size++] = data;
}

void
array_for_each(struct array *a, void (*f)(void *))
{
	size_t i;

	for (i = 0; i < a->size; ++i)
		f(a->items[i]);
}

