#define	NSDHC	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NSDHC
 .global _KERNEL_OPT_NSDHC
 .equiv _KERNEL_OPT_NSDHC,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NSDHC\n .global _KERNEL_OPT_NSDHC\n .equiv _KERNEL_OPT_NSDHC,0x0\n .endif");
#endif
