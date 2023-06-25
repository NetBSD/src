/* option `MACHDEP' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_MACHDEP
 .global _KERNEL_OPT_MACHDEP
 .equiv _KERNEL_OPT_MACHDEP,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_MACHDEP\n .global _KERNEL_OPT_MACHDEP\n .equiv _KERNEL_OPT_MACHDEP,0x6e074def\n .endif");
#endif
