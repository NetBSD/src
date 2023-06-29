#define	NMPLS	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NMPLS
 .global _KERNEL_OPT_NMPLS
 .equiv _KERNEL_OPT_NMPLS,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NMPLS\n .global _KERNEL_OPT_NMPLS\n .equiv _KERNEL_OPT_NMPLS,0x0\n .endif");
#endif
