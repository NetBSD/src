/* option `LACP_NOFDX' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_LACP_NOFDX
 .global _KERNEL_OPT_LACP_NOFDX
 .equiv _KERNEL_OPT_LACP_NOFDX,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_LACP_NOFDX\n .global _KERNEL_OPT_LACP_NOFDX\n .equiv _KERNEL_OPT_LACP_NOFDX,0x6e074def\n .endif");
#endif
/* option `LACP_STANDBY_SYNCED' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_LACP_STANDBY_SYNCED
 .global _KERNEL_OPT_LACP_STANDBY_SYNCED
 .equiv _KERNEL_OPT_LACP_STANDBY_SYNCED,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_LACP_STANDBY_SYNCED\n .global _KERNEL_OPT_LACP_STANDBY_SYNCED\n .equiv _KERNEL_OPT_LACP_STANDBY_SYNCED,0x6e074def\n .endif");
#endif
/* option `LACP_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_LACP_DEBUG
 .global _KERNEL_OPT_LACP_DEBUG
 .equiv _KERNEL_OPT_LACP_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_LACP_DEBUG\n .global _KERNEL_OPT_LACP_DEBUG\n .equiv _KERNEL_OPT_LACP_DEBUG,0x6e074def\n .endif");
#endif
/* option `LAGG_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_LAGG_DEBUG
 .global _KERNEL_OPT_LAGG_DEBUG
 .equiv _KERNEL_OPT_LAGG_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_LAGG_DEBUG\n .global _KERNEL_OPT_LAGG_DEBUG\n .equiv _KERNEL_OPT_LAGG_DEBUG,0x6e074def\n .endif");
#endif
/* option `LAGG_SETCAPS_RETRY' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_LAGG_SETCAPS_RETRY
 .global _KERNEL_OPT_LAGG_SETCAPS_RETRY
 .equiv _KERNEL_OPT_LAGG_SETCAPS_RETRY,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_LAGG_SETCAPS_RETRY\n .global _KERNEL_OPT_LAGG_SETCAPS_RETRY\n .equiv _KERNEL_OPT_LAGG_SETCAPS_RETRY,0x6e074def\n .endif");
#endif
