#define	NAC100IC	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NAC100IC
 .global _KERNEL_OPT_NAC100IC
 .equiv _KERNEL_OPT_NAC100IC,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NAC100IC\n .global _KERNEL_OPT_NAC100IC\n .equiv _KERNEL_OPT_NAC100IC,0x0\n .endif");
#endif
