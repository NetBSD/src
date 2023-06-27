#define	NGENFB	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NGENFB
 .global _KERNEL_OPT_NGENFB
 .equiv _KERNEL_OPT_NGENFB,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NGENFB\n .global _KERNEL_OPT_NGENFB\n .equiv _KERNEL_OPT_NGENFB,0x0\n .endif");
#endif
