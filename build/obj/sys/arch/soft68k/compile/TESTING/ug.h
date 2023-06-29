#define	NUG	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NUG
 .global _KERNEL_OPT_NUG
 .equiv _KERNEL_OPT_NUG,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NUG\n .global _KERNEL_OPT_NUG\n .equiv _KERNEL_OPT_NUG,0x0\n .endif");
#endif
