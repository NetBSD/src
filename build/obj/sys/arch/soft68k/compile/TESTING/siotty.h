#define	NSIOTTY	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NSIOTTY
 .global _KERNEL_OPT_NSIOTTY
 .equiv _KERNEL_OPT_NSIOTTY,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NSIOTTY\n .global _KERNEL_OPT_NSIOTTY\n .equiv _KERNEL_OPT_NSIOTTY,0x1\n .endif");
#endif
