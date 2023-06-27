#define	NAGR	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NAGR
 .global _KERNEL_OPT_NAGR
 .equiv _KERNEL_OPT_NAGR,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NAGR\n .global _KERNEL_OPT_NAGR\n .equiv _KERNEL_OPT_NAGR,0x1\n .endif");
#endif
