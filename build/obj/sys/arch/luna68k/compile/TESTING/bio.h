#define	NBIO	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NBIO
 .global _KERNEL_OPT_NBIO
 .equiv _KERNEL_OPT_NBIO,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NBIO\n .global _KERNEL_OPT_NBIO\n .equiv _KERNEL_OPT_NBIO,0x0\n .endif");
#endif
