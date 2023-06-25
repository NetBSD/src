#define	NWSFONT	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NWSFONT
 .global _KERNEL_OPT_NWSFONT
 .equiv _KERNEL_OPT_NWSFONT,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NWSFONT\n .global _KERNEL_OPT_NWSFONT\n .equiv _KERNEL_OPT_NWSFONT,0x0\n .endif");
#endif
