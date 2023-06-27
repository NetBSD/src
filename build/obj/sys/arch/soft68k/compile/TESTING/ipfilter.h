#define	NIPFILTER	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NIPFILTER
 .global _KERNEL_OPT_NIPFILTER
 .equiv _KERNEL_OPT_NIPFILTER,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NIPFILTER\n .global _KERNEL_OPT_NIPFILTER\n .equiv _KERNEL_OPT_NIPFILTER,0x0\n .endif");
#endif
