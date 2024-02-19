#ifndef MAL_H
#define MAL_H

#define MAL_KB(n) (1024*(n))
#define MAL_MB(n) (1024*MAL_KB(n))
#define MAL_GB(n) (1024*MAL_MB(n))

#define SIM_PAGE_SIZE MAL_KB(4)

// NOTE: Starting out with page grab simulator before using OS specific api

// TODO: Linear allocator
// TODO: Fixed size block allocator (pool allocator)
// TODO: General size block allocator
// TODO: Abstract allocator properties to allow customization
// TODO: Abstract allocator properties to create specific allocator variations

#endif //MAL_H

#ifdef MAL_IMPLEMENTATION

#endif //MAL_IMPLEMENTATION
