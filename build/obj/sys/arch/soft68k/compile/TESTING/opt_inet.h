/* option `IPSELSRC' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_IPSELSRC
 .global _KERNEL_OPT_IPSELSRC
 .equiv _KERNEL_OPT_IPSELSRC,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_IPSELSRC\n .global _KERNEL_OPT_IPSELSRC\n .equiv _KERNEL_OPT_IPSELSRC,0x6e074def\n .endif");
#endif
/* option `TCP_REASS_COUNTERS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_TCP_REASS_COUNTERS
 .global _KERNEL_OPT_TCP_REASS_COUNTERS
 .equiv _KERNEL_OPT_TCP_REASS_COUNTERS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_TCP_REASS_COUNTERS\n .global _KERNEL_OPT_TCP_REASS_COUNTERS\n .equiv _KERNEL_OPT_TCP_REASS_COUNTERS,0x6e074def\n .endif");
#endif
/* option `TCP_OUTPUT_COUNTERS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_TCP_OUTPUT_COUNTERS
 .global _KERNEL_OPT_TCP_OUTPUT_COUNTERS
 .equiv _KERNEL_OPT_TCP_OUTPUT_COUNTERS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_TCP_OUTPUT_COUNTERS\n .global _KERNEL_OPT_TCP_OUTPUT_COUNTERS\n .equiv _KERNEL_OPT_TCP_OUTPUT_COUNTERS,0x6e074def\n .endif");
#endif
/* option `TCP_SIGNATURE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_TCP_SIGNATURE
 .global _KERNEL_OPT_TCP_SIGNATURE
 .equiv _KERNEL_OPT_TCP_SIGNATURE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_TCP_SIGNATURE\n .global _KERNEL_OPT_TCP_SIGNATURE\n .equiv _KERNEL_OPT_TCP_SIGNATURE,0x6e074def\n .endif");
#endif
#define	INET6	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_INET6
 .global _KERNEL_OPT_INET6
 .equiv _KERNEL_OPT_INET6,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_INET6\n .global _KERNEL_OPT_INET6\n .equiv _KERNEL_OPT_INET6,0x1\n .endif");
#endif
#define	INET	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_INET
 .global _KERNEL_OPT_INET
 .equiv _KERNEL_OPT_INET,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_INET\n .global _KERNEL_OPT_INET\n .equiv _KERNEL_OPT_INET,0x1\n .endif");
#endif
