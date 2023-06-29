/* option `KERNHIST_PRINT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KERNHIST_PRINT
 .global _KERNEL_OPT_KERNHIST_PRINT
 .equiv _KERNEL_OPT_KERNHIST_PRINT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KERNHIST_PRINT\n .global _KERNEL_OPT_KERNHIST_PRINT\n .equiv _KERNEL_OPT_KERNHIST_PRINT,0x6e074def\n .endif");
#endif
/* option `KERNHIST' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KERNHIST
 .global _KERNEL_OPT_KERNHIST
 .equiv _KERNEL_OPT_KERNHIST,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KERNHIST\n .global _KERNEL_OPT_KERNHIST\n .equiv _KERNEL_OPT_KERNHIST,0x6e074def\n .endif");
#endif
/* option `KERNHIST_DELAY' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KERNHIST_DELAY
 .global _KERNEL_OPT_KERNHIST_DELAY
 .equiv _KERNEL_OPT_KERNHIST_DELAY,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KERNHIST_DELAY\n .global _KERNEL_OPT_KERNHIST_DELAY\n .equiv _KERNEL_OPT_KERNHIST_DELAY,0x6e074def\n .endif");
#endif
