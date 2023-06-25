#define	NAXP809PM	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NAXP809PM
 .global _KERNEL_OPT_NAXP809PM
 .equiv _KERNEL_OPT_NAXP809PM,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NAXP809PM\n .global _KERNEL_OPT_NAXP809PM\n .equiv _KERNEL_OPT_NAXP809PM,0x0\n .endif");
#endif
