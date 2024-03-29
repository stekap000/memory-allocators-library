#ifndef MAL_H
#define MAL_H

// TODO: Any include that is used now or in the future is subject to being removed
// in order for library to have minimal dependency.

#include <assert.h>
#include <stddef.h>

#define MALAPI static

#define MAL_NOT_IMPLEMENTED(msg) assert(!(msg))
#define MAL_ASSERT_DIVISIBILITY(a, b) assert((a) >= (b) && (a)%(b) == 0)

#define MAL_KB(n) (1024*(n))
#define MAL_MB(n) (1024*MAL_KB(n))
#define MAL_GB(n) (1024*MAL_MB(n))

// For now, OS specific functions return 0 on fail.
#define MAL_ERROR 0
#define MAL_SUCCESS 1
#define MAL_VALID(ret)   ((ret) != MAL_ERROR)
#define MAL_INVALID(ret) ((ret) == MAL_ERROR)
#ifdef MAL_ERROR_ASSERT
#define MAL_ERROR_RETURN(ret) assert(!"MAL_ERROR_ASSERT GUARD")
#else
#define MAL_ERROR_RETURN(ret) return (ret);
#endif

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
	size_t size;
	size_t capacity;
} mal_General_Pool;

typedef struct {
	void *start;
	size_t number_of_taken_slots;
	size_t slot_size;
	size_t capacity;
} mal_Stack;

typedef struct {
	void *start;
	void *tos;
	size_t size;
	size_t capacity;
} mal_General_Stack;

// OS dependent
// ===================================================
MALAPI size_t mal_get_system_page_size();
MALAPI void *mal_raw_alloc(size_t capacity);
MALAPI int mal_raw_free(void *address, size_t length);
// ===================================================

MALAPI size_t mal_ceil_to_page_boundary(size_t size);

MALAPI mal_Arena mal_arena_create(size_t capacity);
MALAPI void *mal_arena_alloc(mal_Arena *arena, size_t size);
MALAPI void mal_arena_reset(mal_Arena *arena);
MALAPI int mal_arena_destroy();

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
MALAPI int mal_pool_destroy(mal_Pool *pool);

MALAPI mal_General_Pool mal_general_create(size_t capacity);
MALAPI void *mal_general_alloc(mal_General_Pool *general_pool, size_t size);
MALAPI void *mal_general_realloc();
MALAPI void mal_general_reset(mal_General_Pool *general_pool);
MALAPI void mal_general_free(mal_General_Pool *general_pool);
MALAPI void mal_general_destroy(mal_General_Pool *general_pool);

MALAPI mal_Stack mal_stack_create(size_t capacity, size_t slot_size);
MALAPI void *mal_stack_alloc(mal_Stack *stack);
MALAPI void mal_stack_reset(mal_Stack *stack);
MALAPI void mal_stack_free(mal_Stack *stack);
MALAPI int mal_stack_destroy(mal_Stack *stack);

MALAPI mal_General_Stack mal_general_stack_create(size_t capacity);
MALAPI void *mal_general_stack_alloc(mal_General_Stack *stack, size_t size);
MALAPI void mal_general_stack_reset(mal_General_Stack *stack);
MALAPI void mal_general_stack_free(mal_General_Stack *stack);
MALAPI int mal_general_stack_destroy(mal_General_Stack *stack);

#endif //MAL_H

#ifdef MAL_IMPLEMENTATION

#if defined(_WIN32)
#include <windows.h> // For system info we just need sysinfoapi.h

// Cached pseudo handle for current process on windows.
static void *_current_process_pseudo_handle = 0;

MALAPI size_t mal_get_system_page_size() {
	static size_t _mal_system_page_size = 0;
	if(_mal_system_page_size != 0) return _mal_system_page_size;

	SYSTEM_INFO si = {0};
	GetSystemInfo(&si);
	_mal_system_page_size = si.dwPageSize;
	return _mal_system_page_size;
}

MALAPI void *mal_raw_alloc(size_t capacity) {
	if(_current_process_pseudo_handle == 0)
		_current_process_pseudo_handle = GetCurrentProcess();
	
	void *address = VirtualAllocEx(_current_process_pseudo_handle,
								   0,
								   capacity,
								   MEM_COMMIT | MEM_RESERVE,
								   PAGE_READWRITE);
	if(address == 0) MAL_ERROR_RETURN(MAL_ERROR);
	return address;
}

MALAPI int mal_raw_free(void *address, size_t length) {
	if(_current_process_pseudo_handle == 0)
		_current_process_pseudo_handle = GetCurrentProcess();
		
	int ret = VirtualFreeEx(_current_process_pseudo_handle,
							address,
							0,
							MEM_RELEASE);
	if(ret == 0) MAL_ERROR_RETURN(MAL_ERROR);
	return ret;
}

#elif defined(__linux__)
#include <unistd.h>
#include <sys/mman.h>

MALAPI size_t mal_get_system_page_size() {
	static size_t _mal_system_page_size = 0;
	if(_mal_system_page_size != 0) return _mal_system_page_size;
	
	long size = sysconf(_SC_PAGESIZE);
	if(size < 0) MAL_ERROR_RETURN(MAL_ERROR);
	_mal_system_page_size = (size_t)size;
	return _mal_system_page_size;
}

MALAPI void *mal_raw_alloc(size_t capacity) {
	void *addr = mmap(0, capacity,
					  PROT_READ | PROT_WRITE,
					  MAP_ANONYMOUS | MAP_PRIVATE,
					  -1, 0);
	if(addr == MAP_FAILED) MAL_ERROR_RETURN(MAL_ERROR);
	return addr;
}

MALAPI int mal_raw_free(void *address, size_t length) {
	if(munmap(address, length) == -1) MAL_ERROR_RETURN(MAL_ERROR);
	return MAL_SUCCESS;
}

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
	if(MAL_INVALID(arena.start)) MAL_ERROR_RETURN(arena);
	arena.capacity = mal_ceil_to_page_boundary(capacity);
	return arena;
}

MALAPI void *mal_arena_alloc(mal_Arena *arena, size_t size) {
	if(arena->size + size > arena->capacity) MAL_ERROR_RETURN(MAL_ERROR);

	void *item_address = (char *)arena->start + arena->size;
	arena->size += size;
	return item_address;
}

MALAPI void mal_arena_reset(mal_Arena *arena) {
	arena->size = 0;
}

MALAPI int mal_arena_destroy(mal_Arena *arena) {
	int ret = mal_raw_free(arena->start, arena->capacity);
	if(MAL_INVALID(ret)) MAL_ERROR_RETURN(MAL_ERROR);
	arena->start = 0;
	arena->size = 0;
	arena->capacity = 0;
	return MAL_SUCCESS;
}

MALAPI mal_Pool mal_pool_create(size_t capacity, size_t slot_size) {
	if(slot_size < sizeof(size_t)) slot_size = sizeof(size_t);
	
	mal_Pool pool = {0};
	pool.start = mal_raw_alloc(capacity);
	if(MAL_INVALID(pool.start)) MAL_ERROR_RETURN(pool);
	pool.free_start = pool.start;
	pool.capacity = mal_ceil_to_page_boundary(capacity);
	pool.slot_size = slot_size;
	
	MAL_ASSERT_DIVISIBILITY(pool.capacity, pool.slot_size);
	
	return pool;
}

MALAPI void *mal_pool_alloc(mal_Pool *pool) {
	if(pool->number_of_taken_slots * pool->slot_size == pool->capacity) MAL_ERROR_RETURN(MAL_ERROR);
	   
	int offset = *(int *)pool->free_start;
	void *temp = pool->free_start;
	pool->free_start = (byte *)pool->free_start + ((offset + 1)* pool->slot_size);
	pool->number_of_taken_slots++;

	return temp;
}

MALAPI void *mal_pool_realloc() {
	MAL_NOT_IMPLEMENTED("Pool realloc is not implemented yet");
	return 0;
}

MALAPI void mal_pool_reset(mal_Pool *pool) {
	pool->free_start = pool->start;
	pool->number_of_taken_slots = 0;
}

MALAPI void mal_pool_free(mal_Pool *pool, void *address) {
	int diff = ((byte *)pool->free_start - (byte *)address) / (int)pool->slot_size;

	pool->free_start = address;
	*(int *)pool->free_start = diff - 1;

	pool->number_of_taken_slots--;
}

MALAPI int mal_pool_destroy(mal_Pool *pool) {
	int ret = mal_raw_free(pool->start, pool->capacity);
	if(MAL_INVALID(ret)) MAL_ERROR_RETURN(MAL_ERROR);
	pool->start = 0;
	pool->free_start = 0;
	pool->number_of_taken_slots = 0;
	pool->slot_size = 0;
	pool->capacity = 0;
	return MAL_SUCCESS;
}

MALAPI mal_General_Pool mal_general_create(size_t capacity) {
	MAL_NOT_IMPLEMENTED("General allocator create is not implemented yet");
	return (mal_General_Pool){0};
}

MALAPI void *mal_general_alloc(mal_General_Pool *general_pool, size_t size) {
	MAL_NOT_IMPLEMENTED("General allocator alloc is not implemented yet");
	return 0;
}

MALAPI void *mal_general_realloc() {
	MAL_NOT_IMPLEMENTED("General allocator realloc is not implemented yet");
	return 0;
}

MALAPI void mal_general_reset(mal_General_Pool *general_pool) {
	MAL_NOT_IMPLEMENTED("General allocator reset is not implemented yet");
}

MALAPI void mal_general_free(mal_General_Pool *general_pool) {
	MAL_NOT_IMPLEMENTED("General allocator free is not implemented yet");
}

MALAPI void mal_general_destroy(mal_General_Pool *general_pool) {
	MAL_NOT_IMPLEMENTED("General allocator destroy is not implemented yet");
}

MALAPI mal_Stack mal_stack_create(size_t capacity, size_t slot_size) {
	mal_Stack stack = {0};
	stack.start = mal_raw_alloc(capacity);
	if(MAL_INVALID(stack.start)) MAL_ERROR_RETURN(stack);
	stack.capacity = mal_ceil_to_page_boundary(capacity);
	stack.slot_size = slot_size;

	MAL_ASSERT_DIVISIBILITY(stack.capacity, stack.slot_size);
	
	return stack;
}

MALAPI void *mal_stack_alloc(mal_Stack *stack) {
	if(stack->number_of_taken_slots * stack->slot_size == stack->capacity) MAL_ERROR_RETURN(MAL_ERROR);

	void *temp = (byte *)stack->start + stack->number_of_taken_slots * stack->slot_size;
	stack->number_of_taken_slots++;
	return temp;
}

MALAPI void mal_stack_reset(mal_Stack *stack) {
	stack->number_of_taken_slots = 0;
}

MALAPI void mal_stack_free(mal_Stack *stack) {
	if(stack->number_of_taken_slots == 0) return;
	stack->number_of_taken_slots--;
}

MALAPI int mal_stack_destroy(mal_Stack *stack) {
	int ret = mal_raw_free(stack->start, stack->capacity);
	if(MAL_INVALID(ret)) MAL_ERROR_RETURN(MAL_ERROR);
	stack->start = 0;
	stack->number_of_taken_slots = 0;
	stack->slot_size = 0;
	stack->capacity = 0;
	return MAL_SUCCESS;
}

MALAPI mal_General_Stack mal_general_stack_create(size_t capacity) {
	mal_General_Stack stack = {0};
	stack.start = mal_raw_alloc(capacity);
	if(MAL_INVALID(stack.start)) MAL_ERROR_RETURN(stack); 
	stack.tos = stack.start;
	stack.capacity = mal_ceil_to_page_boundary(capacity);;
	return stack;
}

MALAPI void *mal_general_stack_alloc(mal_General_Stack *stack, size_t size) {
	size_t real_size = size + sizeof(size_t);
	
	if(stack->size + real_size > stack->capacity) MAL_ERROR_RETURN(MAL_ERROR);
	
	void *temp = stack->tos;
	stack->tos = (byte *)stack->tos + size;
	*(size_t *)(stack->tos) = (size_t)temp;
	stack->tos = (byte *)stack->tos + sizeof(size_t);

	stack->size += real_size;
	
	return temp;
}
 
MALAPI void mal_general_stack_reset(mal_General_Stack *stack) {
	stack->tos = stack->start;
	stack->size = 0;
}
 
MALAPI void mal_general_stack_free(mal_General_Stack *stack) {
	if(stack->size == 0) return;
	
	size_t temp = (size_t)stack->tos;
	stack->tos = (byte *)stack->tos - sizeof(size_t);
	stack->tos = (void *)(*(size_t *)(stack->tos));
	stack->size -= ((byte *)stack->tos - (byte *)temp);
}
 
MALAPI int mal_general_stack_destroy(mal_General_Stack *stack) {
	int ret = mal_raw_free(stack->start, stack->capacity);
	if(MAL_INVALID(ret)) MAL_ERROR_RETURN(MAL_ERROR);
	stack->start = 0;
	stack->tos = 0;
	stack->size = 0;
	stack->capacity = 0;
	return MAL_SUCCESS;
}

// TODO: Remove these functions (just for testing)
#include <stdio.h>
void mal_print(void *allocator, int num_slots) {
	for(int i = 0; i < num_slots; ++i) {
		printf("%02x %02x %02x %02x\n",
			   *((byte*)((mal_Arena *)allocator)->start + i*4 + 3),
			   *((byte*)((mal_Arena *)allocator)->start + i*4 + 2),
			   *((byte*)((mal_Arena *)allocator)->start + i*4 + 1),
			   *((byte*)((mal_Arena *)allocator)->start + i*4));
	}
}

#endif //MAL_IMPLEMENTATION

// TODO: Maybe memset allocated addresses to zero?
// TODO: Maybe memset things to zero when reset?
// TODO: Check if the given address is valid address that was allocated when using free.
// TODO: Add padding when elements of particular size are allocated.
// TODO: Allow arena and pool grow (maybe allow usage without passing sizes?, although
// this would be annoying to efficiently predict in general case).
// TODO: Abstract allocator properties to allow customization.
// TODO: Abstract allocator properties to create specific allocator variations.

