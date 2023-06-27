/* option `ATPPC_VERBOSE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_ATPPC_VERBOSE
 .global _KERNEL_OPT_ATPPC_VERBOSE
 .equiv _KERNEL_OPT_ATPPC_VERBOSE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_ATPPC_VERBOSE\n .global _KERNEL_OPT_ATPPC_VERBOSE\n .equiv _KERNEL_OPT_ATPPC_VERBOSE,0x6e074def\n .endif");
#endif
/* option `ATPPC_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_ATPPC_DEBUG
 .global _KERNEL_OPT_ATPPC_DEBUG
 .equiv _KERNEL_OPT_ATPPC_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_ATPPC_DEBUG\n .global _KERNEL_OPT_ATPPC_DEBUG\n .equiv _KERNEL_OPT_ATPPC_DEBUG,0x6e074def\n .endif");
#endif
