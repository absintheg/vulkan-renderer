#ifndef FIXED_ALLOCATOR_H
#define FIXED_ALLOCATOR_H

#include "allocator.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	size_t size;
} fixed_allocator_block_header_t;

typedef struct {
	uint8_t *buffer;
	size_t capacity;
	size_t used;
} fixed_allocator_t;

extern allocator_callbacks_t fixed_allocator_callbacks;

bool fixed_allocator_create(
		fixed_allocator_t *out,
		unsigned long capacity,
		allocator_t child_allocator);

allocator_t fixed_allocator_get_allocator(fixed_allocator_t *fixed_allocator);

#endif
