#define	NSES	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NSES
 .global _KERNEL_OPT_NSES
 .equiv _KERNEL_OPT_NSES,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NSES\n .global _KERNEL_OPT_NSES\n .equiv _KERNEL_OPT_NSES,0x0\n .endif");
#endif
