#define __GLIBC__ 3             /* Fool unwind-dw2-fde-glibc.c.  */
#define ElfW(type) Elf_##type
#include "unwind-dw2-fde-glibc.c"
