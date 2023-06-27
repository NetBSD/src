/* option `CNMAGIC' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CNMAGIC
 .global _KERNEL_OPT_CNMAGIC
 .equiv _KERNEL_OPT_CNMAGIC,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CNMAGIC\n .global _KERNEL_OPT_CNMAGIC\n .equiv _KERNEL_OPT_CNMAGIC,0x6e074def\n .endif");
#endif
