/* option `COMPAT_NETBSD32' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_COMPAT_NETBSD32
 .global _KERNEL_OPT_COMPAT_NETBSD32
 .equiv _KERNEL_OPT_COMPAT_NETBSD32,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_COMPAT_NETBSD32\n .global _KERNEL_OPT_COMPAT_NETBSD32\n .equiv _KERNEL_OPT_COMPAT_NETBSD32,0x6e074def\n .endif");
#endif
