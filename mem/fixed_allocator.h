#ifndef FIXED_ALLOCATOR_H
#define FIXED_ALLOCATOR_H

#include "allocator.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	size_t size;
} FiixedAllocatorBlockHeader;

typedef struct {
	uint8_t *buffer;
	size_t capacity;
	size_t used;
} FixedAllocator;

extern AllocatorCallbacks fixed_allocator_callbacks;

bool fixed_allocator_create(
		FixedAllocator *out,
		unsigned long capacity,
		Allocator child_allocator);

Allocator fixed_allocator_get_allocator(FixedAllocator *fixed_allocator);

#endif
