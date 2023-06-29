#define	MQUEUE	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_MQUEUE
 .global _KERNEL_OPT_MQUEUE
 .equiv _KERNEL_OPT_MQUEUE,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_MQUEUE\n .global _KERNEL_OPT_MQUEUE\n .equiv _KERNEL_OPT_MQUEUE,0x1\n .endif");
#endif
