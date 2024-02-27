#ifndef MAL_H
#define MAL_H

#include <stdint.h> // TODO: Maybe remove
#include <assert.h> // TODO: Maybe remove

#define NOT_IMPLEMENTED(msg) assert(!(msg))

#define MALAPI static

#define MAL_KB(n) (1024*(n))
#define MAL_MB(n) (1024*MAL_KB(n))
#define MAL_GB(n) (1024*MAL_MB(n))

typedef unsigned char byte;

typedef struct {
	void *start;
	size_t size;
	size_t capacity;
} mal_Arena;

typedef struct {
	void *start;
	void *free_start;
	size_t number_of_taken_slots;
	size_t slot_size;
	size_t capacity;
} mal_Pool;

typedef struct {
	void *start;
	void *free_start;
	size_t capacity;
} mal_General_Pool;

typedef struct {
	void *start;
	void *tos;
	size_t size;
	size_t capacity;
} mal_Stack;

// OS dependent
//===============================
MALAPI uint32_t mal_get_system_page_size();
// Also does zero initialization.
MALAPI void *mal_raw_alloc(size_t capacity);
MALAPI int mal_raw_free();
//===============================

MALAPI size_t mal_ceil_to_page_boundary(size_t size);

MALAPI mal_Arena mal_arena_create(size_t capacity);
MALAPI void *mal_arena_alloc(mal_Arena *arena, size_t size);
MALAPI void mal_arena_reset(mal_Arena *arena);
MALAPI void mal_arena_destroy();

// Pool uses relative addressing between free slots such that 0 represents
// address of next element, -1 current, 1 second next, etc.
// Reason for 0 being used for next element is that it nicely fits initial
// state where we have all zeros. Thus, every free space already points to
// next free space.
MALAPI mal_Pool mal_pool_create(size_t capacity, size_t item_size);
MALAPI void *mal_pool_alloc(mal_Pool *pool);
MALAPI void *mal_pool_realloc();
MALAPI void mal_pool_reset(mal_Pool *pool);
MALAPI void mal_pool_free(mal_Pool *pool, void *address);
MALAPI void mal_pool_destroy(mal_Pool *pool);

MALAPI mal_General_Pool mal_general_create(size_t capacity);
MALAPI void *mal_general_alloc(mal_General_Pool *general_pool, size_t size);
MALAPI void *mal_general_realloc();
MALAPI void mal_general_reset(mal_General_Pool *general_pool);
MALAPI void mal_general_free(mal_General_Pool *general_pool);
MALAPI void mal_general_destroy(mal_General_Pool *general_pool);

MALAPI mal_Stack mal_stack_create(size_t capacity);
MALAPI void *mal_stack_alloc(mal_Stack *stack, size_t size);
MALAPI void mal_stack_reset(mal_Stack *stack);
MALAPI void mal_stack_free(mal_Stack *stack);
MALAPI void mal_stack_destroy(mal_Stack *stack);

#endif //MAL_H

#ifdef MAL_IMPLEMENTATION

#if defined(_WIN32)
#include <windows.h> // For system info we just need sysinfoapi.h

// TODO: Abstract GetCurrentProcess so that the return value can be cached to avoid
// unnecessary calls.
// It returns HANDLE, which is void *.
// See what linux equivalent returns and then decide what should be overarching type.

MALAPI size_t mal_get_system_page_size() {
	static size_t _mal_system_page_size = 0;
	
	if(_mal_system_page_size != 0) return _mal_system_page_size;

	SYSTEM_INFO si = {0};
	GetSystemInfo(&si);
	_mal_system_page_size = si.dwPageSize;
	return _mal_system_page_size;
}

MALAPI void *mal_raw_alloc(size_t capacity) {
	return VirtualAllocEx(GetCurrentProcess(),
						  0, // Possibly adjust later
						  capacity,
						  MEM_COMMIT | MEM_RESERVE,
						  PAGE_READWRITE);
}

MALAPI int mal_raw_free(void *address) {
	return VirtualFreeEx(GetCurrentProcess(),
						 address,
						 0,
						 MEM_RELEASE);
}

#elif defined(__linux__)
// TODO: Linux implementation of OS specific functions
MALAPI size_t mal_get_system_page_size() {
	NOT_IMPLEMENTED("Linux page size not implemented yet.");
	return 0;
}

MALAPI void *mal_raw_alloc(size_t capacity) {
	NOT_IMPLEMENTED("Linux pages alloc not implemented yet.");
	return 0;
}

MALAPI int mal_raw_free(void *address) {
	NOT_IMPLEMENTED("Linux pages free not implemented yet.");
	return 0;

#endif // OS definitions

MALAPI size_t mal_ceil_to_page_boundary(size_t size) {
	size_t page_size = mal_get_system_page_size();
	size_t new_size = (size / page_size) * page_size;
	new_size += ((size % page_size) > 0) * page_size;
	return new_size;
}

MALAPI mal_Arena mal_arena_create(size_t capacity){
	mal_Arena arena = {0};
	arena.start = mal_raw_alloc(capacity);
	// For now, return empty arena if page allocation fails
	if(arena.start == 0) return arena;
	arena.capacity = mal_ceil_to_page_boundary(capacity);
	return arena;
}

MALAPI void *mal_arena_alloc(mal_Arena *arena, size_t size) {
	if(arena->size + size > arena->capacity) return 0;

	void *item_address = (char *)arena->start + arena->size;
	arena->size += size;
	return item_address;
}

MALAPI void mal_arena_reset(mal_Arena *arena) {
	arena->size = 0;
}

MALAPI void mal_arena_destroy(mal_Arena *arena) {
	// TODO: Maybe introduce check for the case when OS page free fails.
	mal_raw_free(arena->start);
	arena->start = 0;
	arena->size = 0;
	arena->capacity = 0;
}

MALAPI mal_Pool mal_pool_create(size_t capacity, size_t slot_size) {
	// TODO: Force slot_size to be divisor of capacity

	if(slot_size < sizeof(size_t)) slot_size = sizeof(size_t);
	
	mal_Pool pool = {0};
	pool.start = mal_raw_alloc(capacity);
	if(pool.start == 0) return pool;
	pool.free_start = pool.start;
	pool.capacity = mal_ceil_to_page_boundary(capacity);
	pool.slot_size = slot_size;
	return pool;
}

MALAPI void *mal_pool_alloc(mal_Pool *pool) {
	if(pool->number_of_taken_slots * pool->slot_size == pool->capacity) return 0;
	   
	int offset = *(int *)pool->free_start;
	void *temp = pool->free_start;
	pool->free_start = (byte *)pool->free_start + ((offset + 1)* pool->slot_size);
	pool->number_of_taken_slots++;

	// TODO: Maybe memset allocated slot to 0
	
	return temp;
}

MALAPI void *mal_pool_realloc() {
	NOT_IMPLEMENTED("Pool realloc is not implemented yet");
	return 0;
}

MALAPI void mal_pool_reset(mal_Pool *pool) {
	// TODO: Maybe memset everything to zero?
	
	pool->free_start = pool->start;
	pool->number_of_taken_slots = 0;
}

MALAPI void mal_pool_free(mal_Pool *pool, void *address) {
	// TODO: Check if the given address is valid address that was allocated

	int diff = ((byte *)pool->free_start - (byte *)address) / (int)pool->slot_size;

	pool->free_start = address;
	*(int *)pool->free_start = diff - 1;

	pool->number_of_taken_slots--;
}

MALAPI void mal_pool_destroy(mal_Pool *pool) {
	mal_raw_free(pool->start);
	pool->start = 0;
	pool->free_start = 0;
	pool->number_of_taken_slots = 0;
	pool->slot_size = 0;
	pool->capacity = 0;
}

MALAPI mal_General_Pool mal_general_create(size_t capacity) {
	NOT_IMPLEMENTED("General allocator create is not implemented yet");
	return (mal_General_Pool){};
}
MALAPI void *mal_general_alloc(mal_General_Pool *general_pool, size_t size) {
	NOT_IMPLEMENTED("General allocator alloc is not implemented yet");
	return 0;
}
MALAPI void *mal_general_realloc() {
	NOT_IMPLEMENTED("General allocator realloc is not implemented yet");
	return 0;
}
MALAPI void mal_general_reset(mal_General_Pool *general_pool) {
	NOT_IMPLEMENTED("General allocator reset is not implemented yet");
}
MALAPI void mal_general_free(mal_General_Pool *general_pool) {
	NOT_IMPLEMENTED("General allocator free is not implemented yet");
}
MALAPI void mal_general_destroy(mal_General_Pool *general_pool) {
	NOT_IMPLEMENTED("General allocator destroy is not implemented yet");
}

MALAPI mal_Stack mal_stack_create(size_t capacity) {
	mal_Stack stack = {0};
	stack.start = mal_raw_alloc(capacity);
	if(stack.start == 0) return stack; 
	stack.tos = stack.start;
	stack.capacity = mal_ceil_to_page_boundary(capacity);;
	
	return stack;
}

MALAPI void *mal_stack_alloc(mal_Stack *stack, size_t size) {
	size_t real_size = size + sizeof(size_t);
	
	if(stack->size + real_size > stack->capacity) return 0;
	
	void *temp = stack->tos;
	stack->tos = (byte *)stack->tos + size;
	*(size_t *)(stack->tos) = (size_t)temp;
	stack->tos = (byte *)stack->tos + sizeof(size_t);

	stack->size += real_size;
	
	return temp;
}
 
MALAPI void mal_stack_reset(mal_Stack *stack) {
	stack->tos = stack->start;
	stack->size = 0;
}
 
MALAPI void mal_stack_free(mal_Stack *stack) {
	if(stack->size == 0) return;
	
	size_t temp = (size_t)stack->tos;
	stack->tos = (byte *)stack->tos - sizeof(size_t);
	stack->tos = (void *)(*(size_t *)(stack->tos));
	stack->size -= ((byte *)stack->tos - (byte *)temp);
}
 
MALAPI void mal_stack_destroy(mal_Stack *stack) {
	mal_raw_free(stack->start);
	stack->start = 0;
	stack->tos = 0;
	stack->size = 0;
	stack->capacity = 0;
}

// TODO: Remove these functions (just for testing)
#include <stdio.h>
void mal_pool_print(mal_Pool *pool, int num_slots) {
	for(int i = 0; i < num_slots; ++i) {
		printf("%02x %02x %02x %02x\n",
			   *((byte*)pool->start + i*4 + 3),
			   *((byte*)pool->start + i*4 + 2),
			   *((byte*)pool->start + i*4 + 1),
			   *((byte*)pool->start + i*4));
		//printf("%p %x\n", (int *)pool->start + i, *((int *)pool->start + i));
	}
}

void mal_stack_print(mal_Stack *stack, int num_slots) {
	for(int i = 0; i < num_slots; ++i) {
		printf("%02x %02x %02x %02x\n",
			   *((byte*)stack->start + i*4 + 3),
			   *((byte*)stack->start + i*4 + 2),
			   *((byte*)stack->start + i*4 + 1),
			   *((byte*)stack->start + i*4));
		//printf("%p %x\n", (int *)pool->start + i, *((int *)pool->start + i));
	}
}

#endif //MAL_IMPLEMENTATION

// NOTE: Try to do it with the least amount of indirection as possible

// TODO: General size block allocator
// TODO: Add padding when elements of particular size are allocated
// TODO: Allow arena and pool grow (maybe allow usage without passing sizes?, although
// this would be annoying to efficiently predict in general case)
// TODO: Abstract allocator properties to allow customization
// TODO: Abstract allocator properties to create specific allocator variations

