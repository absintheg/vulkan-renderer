#include "allocator.h"
#include <stddef.h>
#include <stdlib.h>

size_t align_forward(size_t ptr, size_t align) {
	size_t mod = ptr % align;
	if (mod != 0)
		ptr += (align - mod);
	return ptr;
}

void *std_alloc(void *_self, unsigned long amount) {
	return malloc(amount);
}

void *std_resize(void *_self, unsigned long amount, void *memory) {
	return realloc(memory, amount);
}

void std_free(void *_self, void *memory) {
	free(memory);
}

allocator_callbacks_t std_allocator_callbacks = {
	.alloc = std_alloc,
	.resize = std_resize,
	.free = std_free,
};

allocator_t std_allocator = {
	.callbacks = &std_allocator_callbacks,
	.self = 0,
};

void *allocator_alloc(allocator_t allocator, unsigned long amount) {
	return allocator.callbacks->alloc(allocator.self, amount);
}

void *allocator_resize(allocator_t allocator, unsigned long amount, void *memory) {
	return allocator.callbacks->resize(allocator.self, amount, memory);
}

void allocator_free(allocator_t allocator, void *memory) {
	allocator.callbacks->free(allocator.self, memory);
}
