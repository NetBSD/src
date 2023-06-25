#define	CPU_IN_CKSUM	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CPU_IN_CKSUM
 .global _KERNEL_OPT_CPU_IN_CKSUM
 .equiv _KERNEL_OPT_CPU_IN_CKSUM,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CPU_IN_CKSUM\n .global _KERNEL_OPT_CPU_IN_CKSUM\n .equiv _KERNEL_OPT_CPU_IN_CKSUM,0x1\n .endif");
#endif
