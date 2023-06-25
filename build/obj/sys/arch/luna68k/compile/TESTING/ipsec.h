#define	NIPSEC	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NIPSEC
 .global _KERNEL_OPT_NIPSEC
 .equiv _KERNEL_OPT_NIPSEC,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NIPSEC\n .global _KERNEL_OPT_NIPSEC\n .equiv _KERNEL_OPT_NIPSEC,0x0\n .endif");
#endif
