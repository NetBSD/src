#define	NDSE	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NDSE
 .global _KERNEL_OPT_NDSE
 .equiv _KERNEL_OPT_NDSE,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NDSE\n .global _KERNEL_OPT_NDSE\n .equiv _KERNEL_OPT_NDSE,0x0\n .endif");
#endif
