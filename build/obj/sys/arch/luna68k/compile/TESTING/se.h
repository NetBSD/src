#define	NSE	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NSE
 .global _KERNEL_OPT_NSE
 .equiv _KERNEL_OPT_NSE,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NSE\n .global _KERNEL_OPT_NSE\n .equiv _KERNEL_OPT_NSE,0x0\n .endif");
#endif
