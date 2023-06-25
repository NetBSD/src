#define	COMPAT_43	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_COMPAT_43
 .global _KERNEL_OPT_COMPAT_43
 .equiv _KERNEL_OPT_COMPAT_43,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_COMPAT_43\n .global _KERNEL_OPT_COMPAT_43\n .equiv _KERNEL_OPT_COMPAT_43,0x1\n .endif");
#endif
