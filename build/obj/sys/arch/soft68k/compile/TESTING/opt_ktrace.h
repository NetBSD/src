#define	KTRACE	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KTRACE
 .global _KERNEL_OPT_KTRACE
 .equiv _KERNEL_OPT_KTRACE,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KTRACE\n .global _KERNEL_OPT_KTRACE\n .equiv _KERNEL_OPT_KTRACE,0x1\n .endif");
#endif
