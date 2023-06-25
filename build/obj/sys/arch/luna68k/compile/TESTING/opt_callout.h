/* option `CALLWHEEL_STATS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CALLWHEEL_STATS
 .global _KERNEL_OPT_CALLWHEEL_STATS
 .equiv _KERNEL_OPT_CALLWHEEL_STATS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CALLWHEEL_STATS\n .global _KERNEL_OPT_CALLWHEEL_STATS\n .equiv _KERNEL_OPT_CALLWHEEL_STATS,0x6e074def\n .endif");
#endif
