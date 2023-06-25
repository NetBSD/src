#define	NOFNET	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NOFNET
 .global _KERNEL_OPT_NOFNET
 .equiv _KERNEL_OPT_NOFNET,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NOFNET\n .global _KERNEL_OPT_NOFNET\n .equiv _KERNEL_OPT_NOFNET,0x0\n .endif");
#endif
