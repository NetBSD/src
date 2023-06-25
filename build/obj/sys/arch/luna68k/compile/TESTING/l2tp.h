#define	NL2TP	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NL2TP
 .global _KERNEL_OPT_NL2TP
 .equiv _KERNEL_OPT_NL2TP,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NL2TP\n .global _KERNEL_OPT_NL2TP\n .equiv _KERNEL_OPT_NL2TP,0x0\n .endif");
#endif
