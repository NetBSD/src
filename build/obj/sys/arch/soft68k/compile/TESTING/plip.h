#define	NPLIP	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NPLIP
 .global _KERNEL_OPT_NPLIP
 .equiv _KERNEL_OPT_NPLIP,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NPLIP\n .global _KERNEL_OPT_NPLIP\n .equiv _KERNEL_OPT_NPLIP,0x0\n .endif");
#endif
