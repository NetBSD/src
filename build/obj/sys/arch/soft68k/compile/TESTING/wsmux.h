#define	NWSMUX	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NWSMUX
 .global _KERNEL_OPT_NWSMUX
 .equiv _KERNEL_OPT_NWSMUX,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NWSMUX\n .global _KERNEL_OPT_NWSMUX\n .equiv _KERNEL_OPT_NWSMUX,0x1\n .endif");
#endif
