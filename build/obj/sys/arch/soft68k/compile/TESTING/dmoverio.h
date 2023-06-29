#define	NDMOVERIO	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NDMOVERIO
 .global _KERNEL_OPT_NDMOVERIO
 .equiv _KERNEL_OPT_NDMOVERIO,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NDMOVERIO\n .global _KERNEL_OPT_NDMOVERIO\n .equiv _KERNEL_OPT_NDMOVERIO,0x0\n .endif");
#endif
