#define	COREDUMP	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_COREDUMP
 .global _KERNEL_OPT_COREDUMP
 .equiv _KERNEL_OPT_COREDUMP,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_COREDUMP\n .global _KERNEL_OPT_COREDUMP\n .equiv _KERNEL_OPT_COREDUMP,0x1\n .endif");
#endif
