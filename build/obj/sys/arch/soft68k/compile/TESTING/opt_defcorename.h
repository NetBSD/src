/* option `DEFCORENAME' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_DEFCORENAME
 .global _KERNEL_OPT_DEFCORENAME
 .equiv _KERNEL_OPT_DEFCORENAME,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_DEFCORENAME\n .global _KERNEL_OPT_DEFCORENAME\n .equiv _KERNEL_OPT_DEFCORENAME,0x6e074def\n .endif");
#endif
