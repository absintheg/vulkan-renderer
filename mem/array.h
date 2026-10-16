#ifndef ARRAY_H
#define ARRAY_H

#include "allocator.h"
#include <stdbool.h>

typedef struct {
	void *data;
	int count;
	int size;
	int capacity;
} Array;

#define TRY_CONDITION(expr) \
	if (!expr) return false;

bool array_allocate(
		int capacity,
		int size,
		Allocator allocator,
		Array *out);

void array_free(
		Array *array,
		Allocator allocator);

void *array_get_last    (Array *array);
void *array_get_item    (Array *array, int i);
void *array_append_item (Array *array);
void *array_clear_all   (Array *array);

#endif
