#define	NUHCI	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NUHCI
 .global _KERNEL_OPT_NUHCI
 .equiv _KERNEL_OPT_NUHCI,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NUHCI\n .global _KERNEL_OPT_NUHCI\n .equiv _KERNEL_OPT_NUHCI,0x0\n .endif");
#endif
