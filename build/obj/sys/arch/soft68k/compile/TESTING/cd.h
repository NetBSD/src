#define	NCD	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NCD
 .global _KERNEL_OPT_NCD
 .equiv _KERNEL_OPT_NCD,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NCD\n .global _KERNEL_OPT_NCD\n .equiv _KERNEL_OPT_NCD,0x1\n .endif");
#endif
