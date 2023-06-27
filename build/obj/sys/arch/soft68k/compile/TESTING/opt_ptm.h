#define	COMPAT_BSDPTY	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_COMPAT_BSDPTY
 .global _KERNEL_OPT_COMPAT_BSDPTY
 .equiv _KERNEL_OPT_COMPAT_BSDPTY,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_COMPAT_BSDPTY\n .global _KERNEL_OPT_COMPAT_BSDPTY\n .equiv _KERNEL_OPT_COMPAT_BSDPTY,0x1\n .endif");
#endif
/* option `NO_DEV_PTM' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NO_DEV_PTM
 .global _KERNEL_OPT_NO_DEV_PTM
 .equiv _KERNEL_OPT_NO_DEV_PTM,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NO_DEV_PTM\n .global _KERNEL_OPT_NO_DEV_PTM\n .equiv _KERNEL_OPT_NO_DEV_PTM,0x6e074def\n .endif");
#endif
