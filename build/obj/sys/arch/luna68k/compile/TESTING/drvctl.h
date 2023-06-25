#define	NDRVCTL	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NDRVCTL
 .global _KERNEL_OPT_NDRVCTL
 .equiv _KERNEL_OPT_NDRVCTL,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NDRVCTL\n .global _KERNEL_OPT_NDRVCTL\n .equiv _KERNEL_OPT_NDRVCTL,0x1\n .endif");
#endif
