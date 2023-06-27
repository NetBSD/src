#define	NVHCI	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NVHCI
 .global _KERNEL_OPT_NVHCI
 .equiv _KERNEL_OPT_NVHCI,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NVHCI\n .global _KERNEL_OPT_NVHCI\n .equiv _KERNEL_OPT_NVHCI,0x0\n .endif");
#endif
