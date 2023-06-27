/* option `SIOP_SYMLED' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SIOP_SYMLED
 .global _KERNEL_OPT_SIOP_SYMLED
 .equiv _KERNEL_OPT_SIOP_SYMLED,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SIOP_SYMLED\n .global _KERNEL_OPT_SIOP_SYMLED\n .equiv _KERNEL_OPT_SIOP_SYMLED,0x6e074def\n .endif");
#endif
