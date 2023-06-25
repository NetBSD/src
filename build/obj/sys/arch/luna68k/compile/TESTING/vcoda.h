#define	NVCODA	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NVCODA
 .global _KERNEL_OPT_NVCODA
 .equiv _KERNEL_OPT_NVCODA,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NVCODA\n .global _KERNEL_OPT_NVCODA\n .equiv _KERNEL_OPT_NVCODA,0x0\n .endif");
#endif
