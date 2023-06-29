#define	NSLHCI	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NSLHCI
 .global _KERNEL_OPT_NSLHCI
 .equiv _KERNEL_OPT_NSLHCI,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NSLHCI\n .global _KERNEL_OPT_NSLHCI\n .equiv _KERNEL_OPT_NSLHCI,0x0\n .endif");
#endif
