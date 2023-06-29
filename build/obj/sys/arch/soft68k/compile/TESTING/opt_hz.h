/* option `HZ' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_HZ
 .global _KERNEL_OPT_HZ
 .equiv _KERNEL_OPT_HZ,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_HZ\n .global _KERNEL_OPT_HZ\n .equiv _KERNEL_OPT_HZ,0x6e074def\n .endif");
#endif
