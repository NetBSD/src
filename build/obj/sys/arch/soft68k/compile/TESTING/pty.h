#define	NPTY	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NPTY
 .global _KERNEL_OPT_NPTY
 .equiv _KERNEL_OPT_NPTY,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NPTY\n .global _KERNEL_OPT_NPTY\n .equiv _KERNEL_OPT_NPTY,0x1\n .endif");
#endif
