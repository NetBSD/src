/* option `COMPAT_ULTRIX' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_COMPAT_ULTRIX
 .global _KERNEL_OPT_COMPAT_ULTRIX
 .equiv _KERNEL_OPT_COMPAT_ULTRIX,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_COMPAT_ULTRIX\n .global _KERNEL_OPT_COMPAT_ULTRIX\n .equiv _KERNEL_OPT_COMPAT_ULTRIX,0x6e074def\n .endif");
#endif
