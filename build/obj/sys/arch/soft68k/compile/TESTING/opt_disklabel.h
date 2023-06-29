/* option `DISKLABEL_EI' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DISKLABEL_EI
 .global _KERNEL_OPT_DISKLABEL_EI
 .equiv _KERNEL_OPT_DISKLABEL_EI,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DISKLABEL_EI\n .global _KERNEL_OPT_DISKLABEL_EI\n .equiv _KERNEL_OPT_DISKLABEL_EI,0x6e074def\n .endif");
#endif
