#ifndef MAL_H
#define MAL_H

// TODO: Maybe remove
#include <stdint.h>

#define MALAPI static

#define MAL_KB(n) (1024*(n))
#define MAL_MB(n) (1024*MAL_KB(n))
#define MAL_GB(n) (1024*MAL_MB(n))

typedef struct {
	void *start;
	uint32_t size;
	uint32_t capacity;
} mal_Arena;

typedef struct {
	void *start;
	void *free_start;
	uint32_t number_of_taken_slots;
	uint32_t slot_size;
	uint32_t capacity;
} mal_Pool;

typedef struct {
	void *start;
	void *free_start;
	uint32_t capacity;
} mal_General_Pool;

typedef struct {
	void *start;
	// ...
	uint32_t capacity;
} mal_Stack;

// OS dependent
//===============================
MALAPI uint32_t mal_get_system_page_size();
MALAPI void *mal_raw_alloc(size_t capacity);
MALAPI int mal_raw_free();
//===============================

MALAPI uint32_t mal_ceil_to_page_boundary(uint32_t size);

MALAPI mal_Arena mal_arena_create(uint32_t capacity);
MALAPI void *mal_arena_alloc(mal_Arena *arena, uint32_t size);
MALAPI void mal_arena_reset(mal_Arena *arena);
MALAPI void mal_arena_destroy();

MALAPI mal_Pool mal_pool_create(uint32_t capacity, uint32_t item_size);
MALAPI void *mal_pool_alloc(mal_Pool *pool);
MALAPI void *mal_pool_realloc(); // TODO: Implement
MALAPI void mal_pool_reset(mal_Pool *pool);
MALAPI void mal_pool_free(mal_Pool *pool, void *address);
MALAPI void mal_pool_destroy(mal_Pool *pool);

MALAPI void *mal_general_create(uint32_t capacity);
MALAPI void *mal_general_alloc(mal_General_Pool *general_pool, uint32_t size);
MALAPI void *mal_general_realloc();
MALAPI void mal_general_reset(mal_General_Pool *general_pool);
MALAPI void mal_general_free(mal_General_Pool *general_pool);
MALAPI void mal_general_destroy(mal_General_Pool *general_pool);

MALAPI void *mal_stack_create(uint32_t capacity);
MALAPI void *mal_stack_alloc(mal_Stack *stack, uint32_t size);
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

MALAPI uint32_t mal_get_system_page_size() {
	static uint32_t _mal_system_page_size = 0;
	
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
#endif // OS definitions

MALAPI uint32_t mal_ceil_to_page_boundary(uint32_t size) {
	uint32_t page_size = mal_get_system_page_size();
	uint32_t new_size = (size / page_size) * page_size;
	new_size += ((size % page_size) > 0) * page_size;
	return new_size;
}

MALAPI mal_Arena mal_arena_create(uint32_t capacity){
	mal_Arena arena = {0};
	arena.start = mal_raw_alloc(capacity);
	// For now, return empty arena if page allocation fails
	if(arena.start == 0) return arena;
	arena.capacity = mal_ceil_to_page_boundary(capacity);
	return arena;
}

MALAPI void *mal_arena_alloc(mal_Arena *arena, uint32_t size) {
	if(size > (arena->capacity - arena->size)) return 0;

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

MALAPI mal_Pool mal_pool_create(uint32_t capacity, uint32_t slot_size) {
	// TODO: Force slot_size to be divisor of capacity
	
	// Minimal size for item is the size we need to keep offset to next free element.
	// For now, it is sizeof uint32_t.
	if(slot_size < sizeof(uint32_t)) slot_size = sizeof(uint32_t);
	
	mal_Pool pool = {0};
	pool.start = mal_raw_alloc(capacity);
	// For now, return empty pool if page allocation fails
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
	pool->free_start = (unsigned char *)pool->free_start + ((offset + 1)* pool->slot_size);
	pool->number_of_taken_slots++;

	// TODO: Maybe memset allocated slot to 0
	
	return temp;
}

MALAPI void *mal_pool_realloc() {
	return 0;
}

MALAPI void mal_pool_reset(mal_Pool *pool) {
	// TODO: Maybe memset everything to zero?
	
	pool->free_start = pool->start;
	pool->number_of_taken_slots = 0;
}

MALAPI void mal_pool_free(mal_Pool *pool, void *address) {
	// TODO: Check if the given address is valid address that was allocated

	int diff = ((unsigned char *)pool->free_start - (unsigned char *)address) / pool->slot_size;

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

MALAPI void *mal_general_create(uint32_t capacity);
MALAPI void *mal_general_alloc(mal_General_Pool *general_pool, uint32_t size);
MALAPI void *mal_general_realloc();
MALAPI void mal_general_reset(mal_General_Pool *general_pool);
MALAPI void mal_general_free(mal_General_Pool *general_pool);
MALAPI void mal_general_destroy(mal_General_Pool *general_pool);

MALAPI void *mal_stack_create(uint32_t capacity);
MALAPI void *mal_stack_alloc(mal_Stack *stack, uint32_t size);
MALAPI void mal_stack_reset(mal_Stack *stack);
MALAPI void mal_stack_free(mal_Stack *stack);
MALAPI void mal_stack_destroy(mal_Stack *stack);

// TODO: Remove these functions (just for testing)
#include <stdio.h>
void mal_pool_print(mal_Pool *pool, int num_slots) {
	for(int i = 0; i < num_slots; ++i) {
		printf("%2x %2x %2x %2x\n",
			   *((unsigned char*)pool->start + i*4),
			   *((unsigned char*)pool->start + i*4 + 1),
			   *((unsigned char*)pool->start + i*4 + 2),
			   *((unsigned char*)pool->start + i*4 + 3));
		//printf("%p %x\n", (int *)pool->start + i, *((int *)pool->start + i));
	}
}

#endif //MAL_IMPLEMENTATION

// NOTE: Try to do it with the least amount of indirection as possible

// TODO: General size block allocator

// TODO: Allow arena and pool grow (maybe allow usage without passing sizes?, although
// this would be annoying to efficiently predict in general case)

// TODO: Abstract allocator properties to allow customization
// TODO: Abstract allocator properties to create specific allocator variations

