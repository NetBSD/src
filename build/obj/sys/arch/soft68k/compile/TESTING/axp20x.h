#define	NAXP20X	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NAXP20X
 .global _KERNEL_OPT_NAXP20X
 .equiv _KERNEL_OPT_NAXP20X,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NAXP20X\n .global _KERNEL_OPT_NAXP20X\n .equiv _KERNEL_OPT_NAXP20X,0x0\n .endif");
#endif
