/* option `NOR_VERBOSE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NOR_VERBOSE
 .global _KERNEL_OPT_NOR_VERBOSE
 .equiv _KERNEL_OPT_NOR_VERBOSE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NOR_VERBOSE\n .global _KERNEL_OPT_NOR_VERBOSE\n .equiv _KERNEL_OPT_NOR_VERBOSE,0x6e074def\n .endif");
#endif
/* option `NOR_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NOR_DEBUG
 .global _KERNEL_OPT_NOR_DEBUG
 .equiv _KERNEL_OPT_NOR_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NOR_DEBUG\n .global _KERNEL_OPT_NOR_DEBUG\n .equiv _KERNEL_OPT_NOR_DEBUG,0x6e074def\n .endif");
#endif
