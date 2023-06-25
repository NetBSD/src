/* option `COMPAT_SUNOS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_COMPAT_SUNOS
 .global _KERNEL_OPT_COMPAT_SUNOS
 .equiv _KERNEL_OPT_COMPAT_SUNOS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_COMPAT_SUNOS\n .global _KERNEL_OPT_COMPAT_SUNOS\n .equiv _KERNEL_OPT_COMPAT_SUNOS,0x6e074def\n .endif");
#endif
