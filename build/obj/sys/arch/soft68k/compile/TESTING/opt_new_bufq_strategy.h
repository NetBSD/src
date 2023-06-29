/* option `NEW_BUFQ_STRATEGY' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NEW_BUFQ_STRATEGY
 .global _KERNEL_OPT_NEW_BUFQ_STRATEGY
 .equiv _KERNEL_OPT_NEW_BUFQ_STRATEGY,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NEW_BUFQ_STRATEGY\n .global _KERNEL_OPT_NEW_BUFQ_STRATEGY\n .equiv _KERNEL_OPT_NEW_BUFQ_STRATEGY,0x6e074def\n .endif");
#endif
