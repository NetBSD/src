/* option `STB_IMAGE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_STB_IMAGE
 .global _KERNEL_OPT_STB_IMAGE
 .equiv _KERNEL_OPT_STB_IMAGE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_STB_IMAGE\n .global _KERNEL_OPT_STB_IMAGE\n .equiv _KERNEL_OPT_STB_IMAGE,0x6e074def\n .endif");
#endif
