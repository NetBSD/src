/* option `LPT_VERBOSE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_LPT_VERBOSE
 .global _KERNEL_OPT_LPT_VERBOSE
 .equiv _KERNEL_OPT_LPT_VERBOSE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_LPT_VERBOSE\n .global _KERNEL_OPT_LPT_VERBOSE\n .equiv _KERNEL_OPT_LPT_VERBOSE,0x6e074def\n .endif");
#endif
/* option `LPT_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_LPT_DEBUG
 .global _KERNEL_OPT_LPT_DEBUG
 .equiv _KERNEL_OPT_LPT_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_LPT_DEBUG\n .global _KERNEL_OPT_LPT_DEBUG\n .equiv _KERNEL_OPT_LPT_DEBUG,0x6e074def\n .endif");
#endif
