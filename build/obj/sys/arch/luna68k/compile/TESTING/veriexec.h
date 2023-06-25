#define	NVERIEXEC	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NVERIEXEC
 .global _KERNEL_OPT_NVERIEXEC
 .equiv _KERNEL_OPT_NVERIEXEC,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NVERIEXEC\n .global _KERNEL_OPT_NVERIEXEC\n .equiv _KERNEL_OPT_NVERIEXEC,0x0\n .endif");
#endif
