#define	NWSMOUSE	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NWSMOUSE
 .global _KERNEL_OPT_NWSMOUSE
 .equiv _KERNEL_OPT_NWSMOUSE,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NWSMOUSE\n .global _KERNEL_OPT_NWSMOUSE\n .equiv _KERNEL_OPT_NWSMOUSE,0x1\n .endif");
#endif
