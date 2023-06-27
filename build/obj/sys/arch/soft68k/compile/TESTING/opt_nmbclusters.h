/* option `NMBCLUSTERS_MAX' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NMBCLUSTERS_MAX
 .global _KERNEL_OPT_NMBCLUSTERS_MAX
 .equiv _KERNEL_OPT_NMBCLUSTERS_MAX,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NMBCLUSTERS_MAX\n .global _KERNEL_OPT_NMBCLUSTERS_MAX\n .equiv _KERNEL_OPT_NMBCLUSTERS_MAX,0x6e074def\n .endif");
#endif
/* option `NMBCLUSTERS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NMBCLUSTERS
 .global _KERNEL_OPT_NMBCLUSTERS
 .equiv _KERNEL_OPT_NMBCLUSTERS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NMBCLUSTERS\n .global _KERNEL_OPT_NMBCLUSTERS\n .equiv _KERNEL_OPT_NMBCLUSTERS,0x6e074def\n .endif");
#endif
