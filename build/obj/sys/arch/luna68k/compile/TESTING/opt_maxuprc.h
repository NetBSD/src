/* option `MAXUPRC' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_MAXUPRC
 .global _KERNEL_OPT_MAXUPRC
 .equiv _KERNEL_OPT_MAXUPRC,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_MAXUPRC\n .global _KERNEL_OPT_MAXUPRC\n .equiv _KERNEL_OPT_MAXUPRC,0x6e074def\n .endif");
#endif
