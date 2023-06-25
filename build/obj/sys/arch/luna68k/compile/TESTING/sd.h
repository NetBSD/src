#define	NSD	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NSD
 .global _KERNEL_OPT_NSD
 .equiv _KERNEL_OPT_NSD,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NSD\n .global _KERNEL_OPT_NSD\n .equiv _KERNEL_OPT_NSD,0x1\n .endif");
#endif
