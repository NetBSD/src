#define	RFC2292	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_RFC2292
 .global _KERNEL_OPT_RFC2292
 .equiv _KERNEL_OPT_RFC2292,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_RFC2292\n .global _KERNEL_OPT_RFC2292\n .equiv _KERNEL_OPT_RFC2292,0x1\n .endif");
#endif
