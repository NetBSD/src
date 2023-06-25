/* option `COMPAT_M68K4K' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_COMPAT_M68K4K
 .global _KERNEL_OPT_COMPAT_M68K4K
 .equiv _KERNEL_OPT_COMPAT_M68K4K,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_COMPAT_M68K4K\n .global _KERNEL_OPT_COMPAT_M68K4K\n .equiv _KERNEL_OPT_COMPAT_M68K4K,0x6e074def\n .endif");
#endif
