/* option `COMPAT_FREEBSD' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_COMPAT_FREEBSD
 .global _KERNEL_OPT_COMPAT_FREEBSD
 .equiv _KERNEL_OPT_COMPAT_FREEBSD,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_COMPAT_FREEBSD\n .global _KERNEL_OPT_COMPAT_FREEBSD\n .equiv _KERNEL_OPT_COMPAT_FREEBSD,0x6e074def\n .endif");
#endif
