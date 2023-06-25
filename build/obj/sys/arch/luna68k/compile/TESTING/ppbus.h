#define	NPPBUS	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NPPBUS
 .global _KERNEL_OPT_NPPBUS
 .equiv _KERNEL_OPT_NPPBUS,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NPPBUS\n .global _KERNEL_OPT_NPPBUS\n .equiv _KERNEL_OPT_NPPBUS,0x0\n .endif");
#endif
