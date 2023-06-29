/* option `UDP_CSUM_COUNTERS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_UDP_CSUM_COUNTERS
 .global _KERNEL_OPT_UDP_CSUM_COUNTERS
 .equiv _KERNEL_OPT_UDP_CSUM_COUNTERS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_UDP_CSUM_COUNTERS\n .global _KERNEL_OPT_UDP_CSUM_COUNTERS\n .equiv _KERNEL_OPT_UDP_CSUM_COUNTERS,0x6e074def\n .endif");
#endif
/* option `TCP_CSUM_COUNTERS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_TCP_CSUM_COUNTERS
 .global _KERNEL_OPT_TCP_CSUM_COUNTERS
 .equiv _KERNEL_OPT_TCP_CSUM_COUNTERS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_TCP_CSUM_COUNTERS\n .global _KERNEL_OPT_TCP_CSUM_COUNTERS\n .equiv _KERNEL_OPT_TCP_CSUM_COUNTERS,0x6e074def\n .endif");
#endif
/* option `INET_CSUM_COUNTERS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_INET_CSUM_COUNTERS
 .global _KERNEL_OPT_INET_CSUM_COUNTERS
 .equiv _KERNEL_OPT_INET_CSUM_COUNTERS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_INET_CSUM_COUNTERS\n .global _KERNEL_OPT_INET_CSUM_COUNTERS\n .equiv _KERNEL_OPT_INET_CSUM_COUNTERS,0x6e074def\n .endif");
#endif
