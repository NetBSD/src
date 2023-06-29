#define	NMPU	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NMPU
 .global _KERNEL_OPT_NMPU
 .equiv _KERNEL_OPT_NMPU,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NMPU\n .global _KERNEL_OPT_NMPU\n .equiv _KERNEL_OPT_NMPU,0x0\n .endif");
#endif
