/* option `RTFLUSH_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_RTFLUSH_DEBUG
 .global _KERNEL_OPT_RTFLUSH_DEBUG
 .equiv _KERNEL_OPT_RTFLUSH_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_RTFLUSH_DEBUG\n .global _KERNEL_OPT_RTFLUSH_DEBUG\n .equiv _KERNEL_OPT_RTFLUSH_DEBUG,0x6e074def\n .endif");
#endif
/* option `RTCACHE_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_RTCACHE_DEBUG
 .global _KERNEL_OPT_RTCACHE_DEBUG
 .equiv _KERNEL_OPT_RTCACHE_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_RTCACHE_DEBUG\n .global _KERNEL_OPT_RTCACHE_DEBUG\n .equiv _KERNEL_OPT_RTCACHE_DEBUG,0x6e074def\n .endif");
#endif
