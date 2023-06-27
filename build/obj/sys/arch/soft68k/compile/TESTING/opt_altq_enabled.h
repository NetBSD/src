/* option `ALTQ' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_ALTQ
 .global _KERNEL_OPT_ALTQ
 .equiv _KERNEL_OPT_ALTQ,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_ALTQ\n .global _KERNEL_OPT_ALTQ\n .equiv _KERNEL_OPT_ALTQ,0x6e074def\n .endif");
#endif
