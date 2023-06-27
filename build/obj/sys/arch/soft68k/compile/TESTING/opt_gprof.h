/* option `GPROF' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_GPROF
 .global _KERNEL_OPT_GPROF
 .equiv _KERNEL_OPT_GPROF,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_GPROF\n .global _KERNEL_OPT_GPROF\n .equiv _KERNEL_OPT_GPROF,0x6e074def\n .endif");
#endif
