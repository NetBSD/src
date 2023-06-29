#define	NIPMI	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NIPMI
 .global _KERNEL_OPT_NIPMI
 .equiv _KERNEL_OPT_NIPMI,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NIPMI\n .global _KERNEL_OPT_NIPMI\n .equiv _KERNEL_OPT_NIPMI,0x0\n .endif");
#endif
