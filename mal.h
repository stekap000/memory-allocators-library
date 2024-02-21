#ifndef MAL_H
#define MAL_H

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
	uint32_t number_of_taken_slots;
	uint32_t slot_size;
	uint32_t capacity;
} mal_Pool;
// TODO: One way to keep track is through linked list or some analog over slots
// Another way would be to do linear allocation over fixed slots and to cache
// freed slots so that the can be reused later.

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
MALAPI void mal_arena_delete();

MALAPI mal_Pool mal_pool_create(uint32_t capacity, uint32_t item_size);
MALAPI void *mal_pool_alloc();
MALAPI void *mal_pool_realloc();
MALAPI void mal_pool_reset();
MALAPI void mal_pool_free();
MALAPI void mal_pool_delete(mal_Pool *pool);

MALAPI void *mal_general_alloc();
MALAPI void *mal_general_realloc();
MALAPI void mal_general_reset();
MALAPI void mal_general_free();

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
#endif

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

MALAPI void mal_arena_delete(mal_Arena *arena) {
	// TODO: Maybe introduce check for the case when OS page free fails.
	mal_raw_free(arena->start);
	arena->start = 0;
	arena->size = 0;
	arena->capacity = 0;
}

MALAPI mal_Pool mal_pool_create(uint32_t capacity, uint32_t item_size) {
	mal_Pool pool = {0};
	pool.start = mal_raw_alloc(capacity);
	// For now, return empty arena if page allocation fails
	if(pool.start == 0) return pool;
	pool.capacity = mal_ceil_to_page_boundary(capacity);
	pool.slot_size = item_size;
	return pool;
}

//typedef struct {
//	void *start;
//	uint32_t number_of_taken_slots;
//	uint32_t slot_size;
//	uint32_t capacity;
//} mal_Pool;

MALAPI void *mal_pool_alloc(mal_Pool *pool) {
	
	
	return 0;
}

MALAPI void *mal_pool_realloc() {
	return 0;
}

MALAPI void mal_pool_reset() {
	pool->number_of_taken_slots = 0;
	// TODO: Addionally, we need to reset way of tracking free regions
}

MALAPI void mal_pool_free(mal_Pool *pool, void *address) {
	
}

MALAPI void mal_pool_delete(mal_Pool *pool) {
	mal_raw_free(pool->start);
	pool->start = 0;
	pool->number_of_taken_slots = 0;
	pool->slot_size = 0;
	pool->capacity = 0;
}

#endif //MAL_IMPLEMENTATION

// NOTE: Try to do it with the least amount of indirection as possible

// TODO: Fixed size block allocator (pool allocator)
// TODO: Allow arena and pool grow (maybe allow usage without passing sizes?, although
// this would be annoying to efficiently predict in general case)
// TODO: General size block allocator
// TODO: Abstract allocator properties to allow customization
// TODO: Abstract allocator properties to create specific allocator variations

