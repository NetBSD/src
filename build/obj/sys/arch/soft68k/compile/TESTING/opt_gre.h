/* option `GRE_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_GRE_DEBUG
 .global _KERNEL_OPT_GRE_DEBUG
 .equiv _KERNEL_OPT_GRE_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_GRE_DEBUG\n .global _KERNEL_OPT_GRE_DEBUG\n .equiv _KERNEL_OPT_GRE_DEBUG,0x6e074def\n .endif");
#endif
