#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

typedef struct {
	void *(*alloc)(void *self, unsigned long amount);
	void *(*resize)(void *self, unsigned long amount, void *memory);
	void (*free)(void *self, void *memory);
} AllocatorCallbacks;

typedef struct {
	AllocatorCallbacks *callbacks;
	void                  *self;
} Allocator;

void *allocator_alloc(Allocator allocator, unsigned long amount);
void *allocator_resize(Allocator allocator, unsigned long amount, void *memory);
void allocator_free(Allocator allocator, void *memory);

extern AllocatorCallbacks std_allocator_callbacks;
extern Allocator           std_allocator;

size_t align_forward(size_t ptr, size_t align);

#endif
