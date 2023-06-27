/* option `MAXLWP' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_MAXLWP
 .global _KERNEL_OPT_MAXLWP
 .equiv _KERNEL_OPT_MAXLWP,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_MAXLWP\n .global _KERNEL_OPT_MAXLWP\n .equiv _KERNEL_OPT_MAXLWP,0x6e074def\n .endif");
#endif
