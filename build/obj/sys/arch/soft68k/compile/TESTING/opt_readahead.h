/* option `READAHEAD_STATS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_READAHEAD_STATS
 .global _KERNEL_OPT_READAHEAD_STATS
 .equiv _KERNEL_OPT_READAHEAD_STATS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_READAHEAD_STATS\n .global _KERNEL_OPT_READAHEAD_STATS\n .equiv _KERNEL_OPT_READAHEAD_STATS,0x6e074def\n .endif");
#endif
