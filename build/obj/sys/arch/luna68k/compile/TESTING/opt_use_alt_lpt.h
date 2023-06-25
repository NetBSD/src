/* option `USE_ALT_LPT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_USE_ALT_LPT
 .global _KERNEL_OPT_USE_ALT_LPT
 .equiv _KERNEL_OPT_USE_ALT_LPT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_USE_ALT_LPT\n .global _KERNEL_OPT_USE_ALT_LPT\n .equiv _KERNEL_OPT_USE_ALT_LPT,0x6e074def\n .endif");
#endif
