/* option `COMPAT_VAX1K' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_COMPAT_VAX1K
 .global _KERNEL_OPT_COMPAT_VAX1K
 .equiv _KERNEL_OPT_COMPAT_VAX1K,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_COMPAT_VAX1K\n .global _KERNEL_OPT_COMPAT_VAX1K\n .equiv _KERNEL_OPT_COMPAT_VAX1K,0x6e074def\n .endif");
#endif
