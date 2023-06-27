/* option `DEBUG_1284' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DEBUG_1284
 .global _KERNEL_OPT_DEBUG_1284
 .equiv _KERNEL_OPT_DEBUG_1284,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DEBUG_1284\n .global _KERNEL_OPT_DEBUG_1284\n .equiv _KERNEL_OPT_DEBUG_1284,0x6e074def\n .endif");
#endif
/* option `DONTPROBE_1284' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DONTPROBE_1284
 .global _KERNEL_OPT_DONTPROBE_1284
 .equiv _KERNEL_OPT_DONTPROBE_1284,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DONTPROBE_1284\n .global _KERNEL_OPT_DONTPROBE_1284\n .equiv _KERNEL_OPT_DONTPROBE_1284,0x6e074def\n .endif");
#endif
