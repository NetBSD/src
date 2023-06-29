#define	NDWCTWO	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NDWCTWO
 .global _KERNEL_OPT_NDWCTWO
 .equiv _KERNEL_OPT_NDWCTWO,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NDWCTWO\n .global _KERNEL_OPT_NDWCTWO\n .equiv _KERNEL_OPT_NDWCTWO,0x0\n .endif");
#endif
