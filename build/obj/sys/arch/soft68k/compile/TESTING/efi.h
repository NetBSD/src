#define	NEFI	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NEFI
 .global _KERNEL_OPT_NEFI
 .equiv _KERNEL_OPT_NEFI,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NEFI\n .global _KERNEL_OPT_NEFI\n .equiv _KERNEL_OPT_NEFI,0x0\n .endif");
#endif
