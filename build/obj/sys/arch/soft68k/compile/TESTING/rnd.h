#define	NRND	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NRND
 .global _KERNEL_OPT_NRND
 .equiv _KERNEL_OPT_NRND,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NRND\n .global _KERNEL_OPT_NRND\n .equiv _KERNEL_OPT_NRND,0x1\n .endif");
#endif
