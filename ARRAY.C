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

struct array *array_new(void)
{
    struct array *array = xmalloc(sizeof(struct array));

    array->items = xmalloc(INITIAL_ARRAY_LIMIT * sizeof(void *));
    array->limit = INITIAL_ARRAY_LIMIT;
    array->size = 0;

    return array;
}

void array_free(struct array *array)
{
    free(array->items);
    free(array);
}

void array_add(struct array *array, void *data)
{
    if (array->size == array->limit) {
        size_t new_limit = array->limit * 2;

        if (new_limit < array->limit) {
            xerror("array_add: overflow.");
        }

        array->items = xrealloc(array->items, sizeof(void *) * new_limit);
        array->limit = new_limit;
    }

    array->items[array->size++] = data;
}

void array_for_each(struct array *array, void (*function)(void *))
{
    size_t i;

    for (i = 0; i < array->size; ++i) {
        function(array->items[i]);
    }
}

