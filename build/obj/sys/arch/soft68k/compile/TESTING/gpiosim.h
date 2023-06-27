#define	NGPIOSIM	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NGPIOSIM
 .global _KERNEL_OPT_NGPIOSIM
 .equiv _KERNEL_OPT_NGPIOSIM,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NGPIOSIM\n .global _KERNEL_OPT_NGPIOSIM\n .equiv _KERNEL_OPT_NGPIOSIM,0x0\n .endif");
#endif
