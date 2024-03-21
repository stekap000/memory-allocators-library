## memory-allocators-library

Library contains different memory allocators that operate on the space reserved by specific OS page allocation functions. OS functions are used for page reservation directly, instead of going through malloc or similar functions. Currently, only Windows is supported.

Library is written as header only library (__stb__ style) with additional flags.

### Current list of allocators:
- __Arena__ - Block of memory. Allocation just fills space in order. Free can only be done on the whole block.
- __Pool__ - Memory is divided into slots of the same size. Allocation reserves one slot. Free can be done on individual slots.
- __General Pool__ - Not yet implemented (this one is least important because malloc is always there as a general solution, but will be implemented at some point probably with AVL trees).
- __Stack__ - Memory is divided into slots of the same size, but allocation and freeing behave as stack functions push and pop.
- __General Stack__ - Similar to Stack, but does not contain fixed sized slots. Instead, allocated piece can be of arbitrary size.

### Preprocessor flags
- __MAL_IMPLEMENTATION__ - Set this flag if you want to include definitions on top of declarations. By including this, header file starts behaving as c source file.
- __MAL_ERROR_ASSERT__ - Set this flag if you want potential errors to be handled rigorously ie. assert will end program execution in such cases. When this is not included, then problems like allocation error of system pages are not treated as eliminatory. Instead, it is well defined what is the return value in such cases, and the program can continue execution. In this case, it is up to user to handle those cases. 

