/* option `MAGICLINKS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_MAGICLINKS
 .global _KERNEL_OPT_MAGICLINKS
 .equiv _KERNEL_OPT_MAGICLINKS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_MAGICLINKS\n .global _KERNEL_OPT_MAGICLINKS\n .equiv _KERNEL_OPT_MAGICLINKS,0x6e074def\n .endif");
#endif
