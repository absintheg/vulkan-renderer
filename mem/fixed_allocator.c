#include "fixed_allocator.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const int FIXED_ALIGNMENT = 8;

void *fixed_allocator_alloc(void *ptr, unsigned long amount) {
	fixed_allocator_t *fixed_allocator = (fixed_allocator_t *)ptr;

	size_t aligned = align_forward(fixed_allocator->used, FIXED_ALIGNMENT);
	size_t total = sizeof(fixed_allocator_block_header_t) + amount;

	if (aligned + total > fixed_allocator->capacity)
		return NULL;

	fixed_allocator_block_header_t *header =
		(fixed_allocator_block_header_t*)(fixed_allocator->buffer + aligned);
	header->size = amount;

	void *user_ptr = (void*)(header + 1);
	fixed_allocator->used = aligned + total;
	return user_ptr;
}

void *fixed_allocator_resize(void *ptr, unsigned long amount, void *memory) {
	printf("Not implemented yet");
	return NULL;
}

void fixed_allocator_free(void *ptr, void *memory) {
	fixed_allocator_t *fixed_allocator = (fixed_allocator_t *)ptr;
	fixed_allocator_block_header_t *block_header =
		((fixed_allocator_block_header_t *)memory) - 1;

	uint8_t *block_start = (uint8_t *)block_header;
	size_t block_offset =
		(size_t)(block_start - fixed_allocator->buffer);

	size_t expected_end =
		block_offset + sizeof(fixed_allocator_block_header_t) + block_header->size;

	// Only free if this block is the top allocation
	if (expected_end != fixed_allocator->used) {
		return;
	}

	fixed_allocator->used = block_offset;
}

allocator_callbacks_t fixed_allocator_callbacks = {
	.alloc = fixed_allocator_alloc,
	.resize = fixed_allocator_resize,
	.free = fixed_allocator_free,
};

bool fixed_allocator_create(
		fixed_allocator_t *out,
		unsigned long capacity,
		allocator_t child_allocator)
{
	out->capacity = capacity;
	out->used = 0;
	out->buffer = allocator_alloc(child_allocator, capacity);

	if (!out->buffer) {
		return false;
	}

	return true;
}

allocator_t fixed_allocator_get_allocator(fixed_allocator_t *fixed_allocator) {
	allocator_t allocator = {
		.callbacks = &fixed_allocator_callbacks,
		.self = fixed_allocator,
	};
	return allocator;
}
