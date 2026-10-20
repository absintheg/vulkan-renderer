#include "array.h"
#include <assert.h>

bool array_allocate(int capacity, int size, Allocator allocator, Array *array) {
    void *data = allocator_alloc(allocator, capacity * size);
    if (!data) {
        return false;
    }

    array->data = data;
    array->capacity = capacity;
    array->size = size;

    return true;
}

void array_free(Array *array, Allocator allocator) {
    allocator_free(allocator, array->data);
    array->data = 0;
    array->capacity = 0;
    array->count = 0;
}

void *array_last(Array *array) {
    assert(array->count > 0 && "Array access out of bounds");
    return array->data + (array->count - 1) * array->size;
}

void *array_first(Array *array) {
    assert(array->count > 0 && "Array access out of bounds");
    return array->data;
}

void *array_get(Array *array, int i) {
    assert(array->count > i && i >= 0 && "Array access out of bounds");
    return array->data + i * array->size;
}

void *array_append(Array *array) {
    assert(array->count < array->capacity && "Array full");
    array->count += 1;
    return array_last(array);
}
