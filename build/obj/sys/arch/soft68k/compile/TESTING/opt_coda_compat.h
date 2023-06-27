/* option `CODA_COMPAT_5' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CODA_COMPAT_5
 .global _KERNEL_OPT_CODA_COMPAT_5
 .equiv _KERNEL_OPT_CODA_COMPAT_5,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CODA_COMPAT_5\n .global _KERNEL_OPT_CODA_COMPAT_5\n .equiv _KERNEL_OPT_CODA_COMPAT_5,0x6e074def\n .endif");
#endif
