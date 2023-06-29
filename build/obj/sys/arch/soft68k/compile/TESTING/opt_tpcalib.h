/* option `TPCALIBDEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_TPCALIBDEBUG
 .global _KERNEL_OPT_TPCALIBDEBUG
 .equiv _KERNEL_OPT_TPCALIBDEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_TPCALIBDEBUG\n .global _KERNEL_OPT_TPCALIBDEBUG\n .equiv _KERNEL_OPT_TPCALIBDEBUG,0x6e074def\n .endif");
#endif
