/* option `DSRTC_YEAR_START_2K' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DSRTC_YEAR_START_2K
 .global _KERNEL_OPT_DSRTC_YEAR_START_2K
 .equiv _KERNEL_OPT_DSRTC_YEAR_START_2K,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DSRTC_YEAR_START_2K\n .global _KERNEL_OPT_DSRTC_YEAR_START_2K\n .equiv _KERNEL_OPT_DSRTC_YEAR_START_2K,0x6e074def\n .endif");
#endif
