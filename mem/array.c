#include "array.h"
#include <stdio.h>
#include <assert.h>

bool array_allocate(
		int capacity,
		int size,
		allocator_t allocator,
		array_t *out)
{
	void *data = allocator_alloc(allocator, capacity * size);
	if (!data) {
		return false;
	}

	array_t arr;
	arr.data = data;
	arr.capacity = capacity;
	arr.size = size;
	arr.count = 0;
	*out = arr;

	return true;
}

void array_free(array_t *array, allocator_t allocator) {
	allocator_free(allocator, array->data);
	array->data = 0;
	array->capacity = 0;
	array->count = 0;
}

void *array_get_last(array_t *array) {
	assert(array->count > 0 && "Array access out of bounds");
	return array->data + (array->count - 1) * array->size;
}

void *array_get_item(array_t *array, int i) {
	assert(array->count > i && i >= 0 && "Array access out of bounds");
	return array->data + i * array->size;
}

void *array_append_item(array_t *array) {
	assert(array->count < array->capacity && "Array full");
	array->count += 1;
	return array_get_last(array);
}
