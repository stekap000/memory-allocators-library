#ifndef MAL_H
#define MAL_H

#include <stdint.h>


#include <stdio.h>

#define MALAPI static

#define MAL_KB(n) (1024*(n))
#define MAL_MB(n) (1024*MAL_KB(n))
#define MAL_GB(n) (1024*MAL_MB(n))

#define SIM_PAGE_SIZE MAL_KB(4)

// OS dependant
//===============================
MALAPI uint32_t mal_get_system_page_size();
MALAPI void *mal_pages_alloc(size_t capacity);
MALAPI int mal_pages_free();
//===============================

MALAPI void *mal_linear_alloc();
MALAPI void *mal_linear_realloc();
MALAPI void mal_linear_reset();
MALAPI void mal_linear_free();

MALAPI void *mal_pool_alloc();
MALAPI void *mal_pool_realloc();
MALAPI void mal_pool_reset();
MALAPI void mal_pool_free();

MALAPI void *mal_arena_alloc();
MALAPI void *mal_arena_realloc();
MALAPI void mal_arena_reset();
MALAPI void mal_arena_free();

MALAPI void *mal_general_alloc();
MALAPI void *mal_general_realloc();
MALAPI void mal_general_reset();
MALAPI void mal_general_free();

#if defined(_WIN32)
#include <windows.h> // For system info we just need sysinfoapi.h

// TODO: Abstract GetCurrentProcess so that the return value if cached, just
// lie page size. It returns HANDLE, which is void *.
// See what linux equivalent returns and then decide what should be overarching type.

MALAPI uint32_t mal_get_system_page_size() {
	static uint32_t _mal_system_page_size = 0;
	
	if(_mal_system_page_size != 0) return _mal_system_page_size;

	SYSTEM_INFO si = {0};
	GetSystemInfo(&si);
	_mal_system_page_size = si.dwPageSize;
	return _mal_system_page_size;
}

MALAPI void *mal_pages_alloc(size_t capacity) {
	return VirtualAllocEx(GetCurrentProcess(),
						  0, // Possibly adjust later
						  capacity,
						  MEM_COMMIT | MEM_RESERVE,
						  PAGE_READWRITE);
}

MALAPI int mal_pages_free(void *address) {
	return VirtualFreeEx(GetCurrentProcess(),
						 address,
						 0,
						 MEM_RELEASE);
}

#elif defined(__linux__)
// linux implementation
#else
#endif

// NOTE: Try to do it with the least amount of indirection as possible

// TODO: Linear allocator (freeing can only be done fully as a complete reset)
// TODO: Fixed size block allocator (pool allocator)
// TODO: Arena allocator
// TODO: General size block allocator
// TODO: Abstract allocator properties to allow customization
// TODO: Abstract allocator properties to create specific allocator variations

#endif //MAL_H

#ifdef MAL_IMPLEMENTATION

#endif //MAL_IMPLEMENTATION
