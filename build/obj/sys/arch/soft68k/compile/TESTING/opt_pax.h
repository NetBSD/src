/* option `PAX_ASLR_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PAX_ASLR_DEBUG
 .global _KERNEL_OPT_PAX_ASLR_DEBUG
 .equiv _KERNEL_OPT_PAX_ASLR_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PAX_ASLR_DEBUG\n .global _KERNEL_OPT_PAX_ASLR_DEBUG\n .equiv _KERNEL_OPT_PAX_ASLR_DEBUG,0x6e074def\n .endif");
#endif
/* option `PAX_ASLR_DELTA_PROG_LEN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PAX_ASLR_DELTA_PROG_LEN
 .global _KERNEL_OPT_PAX_ASLR_DELTA_PROG_LEN
 .equiv _KERNEL_OPT_PAX_ASLR_DELTA_PROG_LEN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PAX_ASLR_DELTA_PROG_LEN\n .global _KERNEL_OPT_PAX_ASLR_DELTA_PROG_LEN\n .equiv _KERNEL_OPT_PAX_ASLR_DELTA_PROG_LEN,0x6e074def\n .endif");
#endif
/* option `PAX_ASLR_DELTA_STACK_LEN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PAX_ASLR_DELTA_STACK_LEN
 .global _KERNEL_OPT_PAX_ASLR_DELTA_STACK_LEN
 .equiv _KERNEL_OPT_PAX_ASLR_DELTA_STACK_LEN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PAX_ASLR_DELTA_STACK_LEN\n .global _KERNEL_OPT_PAX_ASLR_DELTA_STACK_LEN\n .equiv _KERNEL_OPT_PAX_ASLR_DELTA_STACK_LEN,0x6e074def\n .endif");
#endif
/* option `PAX_ASLR_DELTA_STACK_LSB' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PAX_ASLR_DELTA_STACK_LSB
 .global _KERNEL_OPT_PAX_ASLR_DELTA_STACK_LSB
 .equiv _KERNEL_OPT_PAX_ASLR_DELTA_STACK_LSB,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PAX_ASLR_DELTA_STACK_LSB\n .global _KERNEL_OPT_PAX_ASLR_DELTA_STACK_LSB\n .equiv _KERNEL_OPT_PAX_ASLR_DELTA_STACK_LSB,0x6e074def\n .endif");
#endif
/* option `PAX_ASLR_DELTA_MMAP_LEN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PAX_ASLR_DELTA_MMAP_LEN
 .global _KERNEL_OPT_PAX_ASLR_DELTA_MMAP_LEN
 .equiv _KERNEL_OPT_PAX_ASLR_DELTA_MMAP_LEN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PAX_ASLR_DELTA_MMAP_LEN\n .global _KERNEL_OPT_PAX_ASLR_DELTA_MMAP_LEN\n .equiv _KERNEL_OPT_PAX_ASLR_DELTA_MMAP_LEN,0x6e074def\n .endif");
#endif
/* option `PAX_ASLR_DELTA_MMAP_LSB' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PAX_ASLR_DELTA_MMAP_LSB
 .global _KERNEL_OPT_PAX_ASLR_DELTA_MMAP_LSB
 .equiv _KERNEL_OPT_PAX_ASLR_DELTA_MMAP_LSB,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PAX_ASLR_DELTA_MMAP_LSB\n .global _KERNEL_OPT_PAX_ASLR_DELTA_MMAP_LSB\n .equiv _KERNEL_OPT_PAX_ASLR_DELTA_MMAP_LSB,0x6e074def\n .endif");
#endif
/* option `PAX_ASLR' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PAX_ASLR
 .global _KERNEL_OPT_PAX_ASLR
 .equiv _KERNEL_OPT_PAX_ASLR,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PAX_ASLR\n .global _KERNEL_OPT_PAX_ASLR\n .equiv _KERNEL_OPT_PAX_ASLR,0x6e074def\n .endif");
#endif
/* option `PAX_SEGVGUARD' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PAX_SEGVGUARD
 .global _KERNEL_OPT_PAX_SEGVGUARD
 .equiv _KERNEL_OPT_PAX_SEGVGUARD,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PAX_SEGVGUARD\n .global _KERNEL_OPT_PAX_SEGVGUARD\n .equiv _KERNEL_OPT_PAX_SEGVGUARD,0x6e074def\n .endif");
#endif
/* option `PAX_MPROTECT_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PAX_MPROTECT_DEBUG
 .global _KERNEL_OPT_PAX_MPROTECT_DEBUG
 .equiv _KERNEL_OPT_PAX_MPROTECT_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PAX_MPROTECT_DEBUG\n .global _KERNEL_OPT_PAX_MPROTECT_DEBUG\n .equiv _KERNEL_OPT_PAX_MPROTECT_DEBUG,0x6e074def\n .endif");
#endif
/* option `PAX_MPROTECT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PAX_MPROTECT
 .global _KERNEL_OPT_PAX_MPROTECT
 .equiv _KERNEL_OPT_PAX_MPROTECT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PAX_MPROTECT\n .global _KERNEL_OPT_PAX_MPROTECT\n .equiv _KERNEL_OPT_PAX_MPROTECT,0x6e074def\n .endif");
#endif
