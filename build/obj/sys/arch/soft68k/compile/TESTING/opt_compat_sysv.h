/* option `COMPAT_SYSV' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_COMPAT_SYSV
 .global _KERNEL_OPT_COMPAT_SYSV
 .equiv _KERNEL_OPT_COMPAT_SYSV,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_COMPAT_SYSV\n .global _KERNEL_OPT_COMPAT_SYSV\n .equiv _KERNEL_OPT_COMPAT_SYSV,0x6e074def\n .endif");
#endif
