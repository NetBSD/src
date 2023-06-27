#define	NSGSMIX	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NSGSMIX
 .global _KERNEL_OPT_NSGSMIX
 .equiv _KERNEL_OPT_NSGSMIX,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NSGSMIX\n .global _KERNEL_OPT_NSGSMIX\n .equiv _KERNEL_OPT_NSGSMIX,0x0\n .endif");
#endif
