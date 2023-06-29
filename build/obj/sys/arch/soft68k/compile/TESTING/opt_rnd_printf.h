/* option `RND_PRINTF' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_RND_PRINTF
 .global _KERNEL_OPT_RND_PRINTF
 .equiv _KERNEL_OPT_RND_PRINTF,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_RND_PRINTF\n .global _KERNEL_OPT_RND_PRINTF\n .equiv _KERNEL_OPT_RND_PRINTF,0x6e074def\n .endif");
#endif
