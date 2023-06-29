/* option `AHC_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_AHC_DEBUG
 .global _KERNEL_OPT_AHC_DEBUG
 .equiv _KERNEL_OPT_AHC_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_AHC_DEBUG\n .global _KERNEL_OPT_AHC_DEBUG\n .equiv _KERNEL_OPT_AHC_DEBUG,0x6e074def\n .endif");
#endif
/* option `AHC_NO_TAGS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_AHC_NO_TAGS
 .global _KERNEL_OPT_AHC_NO_TAGS
 .equiv _KERNEL_OPT_AHC_NO_TAGS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_AHC_NO_TAGS\n .global _KERNEL_OPT_AHC_NO_TAGS\n .equiv _KERNEL_OPT_AHC_NO_TAGS,0x6e074def\n .endif");
#endif
