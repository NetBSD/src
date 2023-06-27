#define	NGIF	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NGIF
 .global _KERNEL_OPT_NGIF
 .equiv _KERNEL_OPT_NGIF,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NGIF\n .global _KERNEL_OPT_NGIF\n .equiv _KERNEL_OPT_NGIF,0x0\n .endif");
#endif
