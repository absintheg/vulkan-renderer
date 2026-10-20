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

bool array_allocate(
		int capacity,
		int size,
		Allocator allocator,
		Array *out);

void array_free(
		Array *array,
		Allocator allocator);

void *array_last    (Array *array);
void *array_first   (Array *array);
void *array_get    (Array *array, int i);
void *array_append (Array *array);
void *array_clear   (Array *array);

#endif
