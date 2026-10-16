#ifndef ARRAY_H
#define ARRAY_H

#include "allocator.h"
#include <stdbool.h>

typedef struct {
	void *data;
	int count;
	int size;
	int capacity;
} array_t;

#define TRY_CONDITION(expr) \
	if (!expr) return false;

bool array_allocate(
		int capacity,
		int size,
		allocator_t allocator,
		array_t *out);

void array_free(
		array_t *array,
		allocator_t allocator);

void *array_get_last    (array_t *array);
void *array_get_item    (array_t *array, int i);
void *array_append_item (array_t *array);
void *array_clear_all   (array_t *array);

#endif
