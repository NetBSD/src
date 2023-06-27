#define	NMLX	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NMLX
 .global _KERNEL_OPT_NMLX
 .equiv _KERNEL_OPT_NMLX,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NMLX\n .global _KERNEL_OPT_NMLX\n .equiv _KERNEL_OPT_NMLX,0x0\n .endif");
#endif
