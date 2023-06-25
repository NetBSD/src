#define	NMOTG	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NMOTG
 .global _KERNEL_OPT_NMOTG
 .equiv _KERNEL_OPT_NMOTG,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NMOTG\n .global _KERNEL_OPT_NMOTG\n .equiv _KERNEL_OPT_NMOTG,0x0\n .endif");
#endif
