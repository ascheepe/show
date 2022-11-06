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
#include "vector.h"

struct vector *vector_new(void)
{
    struct vector *vector;

    vector = xmalloc(sizeof(*vector));
    vector->items = xmalloc(INITIAL_ARRAY_LIMIT * sizeof(void *));
    vector->capacity = INITIAL_ARRAY_LIMIT;
    vector->size = 0;

    return vector;
}

void vector_free(struct vector *vector)
{
    free(vector->items);
    free(vector);
}

void vector_add(struct vector *vector, void *data)
{
    if (vector->size == vector->capacity) {
        size_t new_capacity = vector->capacity * 3 / 2;

        vector->items = xrealloc(vector->items,
                                 sizeof(void *) * new_capacity);
        vector->capacity = new_capacity;
    }

    vector->items[vector->size++] = data;
}

void vector_foreach(struct vector *vector, void (*function)(void *))
{
    size_t i;

    for (i = 0; i < vector->size; ++i) {
        function(vector->items[i]);
    }
}

