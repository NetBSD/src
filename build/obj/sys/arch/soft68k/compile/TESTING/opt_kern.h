/* option `KERN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KERN
 .global _KERNEL_OPT_KERN
 .equiv _KERNEL_OPT_KERN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KERN\n .global _KERNEL_OPT_KERN\n .equiv _KERNEL_OPT_KERN,0x6e074def\n .endif");
#endif
