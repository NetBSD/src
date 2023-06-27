#define	FPSP	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_FPSP
 .global _KERNEL_OPT_FPSP
 .equiv _KERNEL_OPT_FPSP,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_FPSP\n .global _KERNEL_OPT_FPSP\n .equiv _KERNEL_OPT_FPSP,0x1\n .endif");
#endif
