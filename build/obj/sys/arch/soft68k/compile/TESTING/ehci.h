#define	NEHCI	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NEHCI
 .global _KERNEL_OPT_NEHCI
 .equiv _KERNEL_OPT_NEHCI,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NEHCI\n .global _KERNEL_OPT_NEHCI\n .equiv _KERNEL_OPT_NEHCI,0x0\n .endif");
#endif
