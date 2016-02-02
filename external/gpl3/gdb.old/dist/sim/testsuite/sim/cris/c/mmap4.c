/* Just check that MAP_DENYWRITE is "honored" (ignored).
#notarget: cris*-*-elf
*/
#define MMAP_FLAGS (MAP_PRIVATE|MAP_DENYWRITE)
#include "mmap1.c"
