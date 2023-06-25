#define	NKERNFS	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NKERNFS
 .global _KERNEL_OPT_NKERNFS
 .equiv _KERNEL_OPT_NKERNFS,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NKERNFS\n .global _KERNEL_OPT_NKERNFS\n .equiv _KERNEL_OPT_NKERNFS,0x1\n .endif");
#endif
