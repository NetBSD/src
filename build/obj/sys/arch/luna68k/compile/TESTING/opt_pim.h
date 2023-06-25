/* option `PIM' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PIM
 .global _KERNEL_OPT_PIM
 .equiv _KERNEL_OPT_PIM,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PIM\n .global _KERNEL_OPT_PIM\n .equiv _KERNEL_OPT_PIM,0x6e074def\n .endif");
#endif
