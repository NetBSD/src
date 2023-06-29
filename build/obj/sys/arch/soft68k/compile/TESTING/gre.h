#define	NGRE	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NGRE
 .global _KERNEL_OPT_NGRE
 .equiv _KERNEL_OPT_NGRE,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NGRE\n .global _KERNEL_OPT_NGRE\n .equiv _KERNEL_OPT_NGRE,0x0\n .endif");
#endif
