#define	NLD	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NLD
 .global _KERNEL_OPT_NLD
 .equiv _KERNEL_OPT_NLD,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NLD\n .global _KERNEL_OPT_NLD\n .equiv _KERNEL_OPT_NLD,0x0\n .endif");
#endif
