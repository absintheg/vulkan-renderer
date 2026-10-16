#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

typedef struct {
	void *(*alloc)(void *self, unsigned long amount);
	void *(*resize)(void *self, unsigned long amount, void *memory);
	void (*free)(void *self, void *memory);
} allocator_callbacks_t;

typedef struct {
	allocator_callbacks_t *callbacks;
	void                  *self;
} allocator_t;

void *allocator_alloc(allocator_t allocator, unsigned long amount);
void *allocator_resize(allocator_t allocator, unsigned long amount, void *memory);
void allocator_free(allocator_t allocator, void *memory);

extern allocator_callbacks_t std_allocator_callbacks;
extern allocator_t           std_allocator;

size_t align_forward(size_t ptr, size_t align);

#endif
