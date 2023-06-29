#define	NXHCI	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NXHCI
 .global _KERNEL_OPT_NXHCI
 .equiv _KERNEL_OPT_NXHCI,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NXHCI\n .global _KERNEL_OPT_NXHCI\n .equiv _KERNEL_OPT_NXHCI,0x0\n .endif");
#endif
