#define	NLCD	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NLCD
 .global _KERNEL_OPT_NLCD
 .equiv _KERNEL_OPT_NLCD,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NLCD\n .global _KERNEL_OPT_NLCD\n .equiv _KERNEL_OPT_NLCD,0x1\n .endif");
#endif
