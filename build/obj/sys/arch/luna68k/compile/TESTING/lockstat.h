#define	NLOCKSTAT	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NLOCKSTAT
 .global _KERNEL_OPT_NLOCKSTAT
 .equiv _KERNEL_OPT_NLOCKSTAT,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NLOCKSTAT\n .global _KERNEL_OPT_NLOCKSTAT\n .equiv _KERNEL_OPT_NLOCKSTAT,0x0\n .endif");
#endif
