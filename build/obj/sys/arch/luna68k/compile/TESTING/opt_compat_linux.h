/* option `COMPAT_LINUX' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_COMPAT_LINUX
 .global _KERNEL_OPT_COMPAT_LINUX
 .equiv _KERNEL_OPT_COMPAT_LINUX,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_COMPAT_LINUX\n .global _KERNEL_OPT_COMPAT_LINUX\n .equiv _KERNEL_OPT_COMPAT_LINUX,0x6e074def\n .endif");
#endif
