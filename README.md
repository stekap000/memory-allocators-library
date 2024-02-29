## memory-allocators-library

Library contains different memory allocators that operate on the space reserved by specific OS page allocation functions. OS functions are used for page reservation directly, instead of going through malloc or similar functions. Currently, only Windows is supported.

### Current list of allocators:
- __Arena__ - Block of memory. Allocation just fills space in order. Free can only be done on the whole block.
- __Pool__ - Memory is divided into slots of the same size. Allocation reserves one slot. Free can be done on individual slots.
- __General Pool__ - Not yet implemented
- __Stack__ - Memory is divided into slots of the same size, but allocation and freeing behave as stack functions push and pop.
- __General Stack__ - Similar to Stack, but does not contain fixed sized slots. Instead, allocated piece can be of arbitrary size.
