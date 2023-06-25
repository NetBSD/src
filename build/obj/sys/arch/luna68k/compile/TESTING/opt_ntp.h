#define	NTP	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NTP
 .global _KERNEL_OPT_NTP
 .equiv _KERNEL_OPT_NTP,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NTP\n .global _KERNEL_OPT_NTP\n .equiv _KERNEL_OPT_NTP,0x1\n .endif");
#endif
/* option `PPS_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PPS_DEBUG
 .global _KERNEL_OPT_PPS_DEBUG
 .equiv _KERNEL_OPT_PPS_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PPS_DEBUG\n .global _KERNEL_OPT_PPS_DEBUG\n .equiv _KERNEL_OPT_PPS_DEBUG,0x6e074def\n .endif");
#endif
/* option `PPS_SYNC' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PPS_SYNC
 .global _KERNEL_OPT_PPS_SYNC
 .equiv _KERNEL_OPT_PPS_SYNC,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PPS_SYNC\n .global _KERNEL_OPT_PPS_SYNC\n .equiv _KERNEL_OPT_PPS_SYNC,0x6e074def\n .endif");
#endif
