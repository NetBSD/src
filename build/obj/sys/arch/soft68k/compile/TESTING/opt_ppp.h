/* option `PPP_FILTER' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PPP_FILTER
 .global _KERNEL_OPT_PPP_FILTER
 .equiv _KERNEL_OPT_PPP_FILTER,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PPP_FILTER\n .global _KERNEL_OPT_PPP_FILTER\n .equiv _KERNEL_OPT_PPP_FILTER,0x6e074def\n .endif");
#endif
/* option `PPP_BSDCOMP' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PPP_BSDCOMP
 .global _KERNEL_OPT_PPP_BSDCOMP
 .equiv _KERNEL_OPT_PPP_BSDCOMP,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PPP_BSDCOMP\n .global _KERNEL_OPT_PPP_BSDCOMP\n .equiv _KERNEL_OPT_PPP_BSDCOMP,0x6e074def\n .endif");
#endif
/* option `PPP_DEFLATE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PPP_DEFLATE
 .global _KERNEL_OPT_PPP_DEFLATE
 .equiv _KERNEL_OPT_PPP_DEFLATE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PPP_DEFLATE\n .global _KERNEL_OPT_PPP_DEFLATE\n .equiv _KERNEL_OPT_PPP_DEFLATE,0x6e074def\n .endif");
#endif
