#define	NIGSFB	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NIGSFB
 .global _KERNEL_OPT_NIGSFB
 .equiv _KERNEL_OPT_NIGSFB,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NIGSFB\n .global _KERNEL_OPT_NIGSFB\n .equiv _KERNEL_OPT_NIGSFB,0x0\n .endif");
#endif
