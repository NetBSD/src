/* option `KCOV' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KCOV
 .global _KERNEL_OPT_KCOV
 .equiv _KERNEL_OPT_KCOV,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KCOV\n .global _KERNEL_OPT_KCOV\n .equiv _KERNEL_OPT_KCOV,0x6e074def\n .endif");
#endif
