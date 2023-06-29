/* option `TCP_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_TCP_DEBUG
 .global _KERNEL_OPT_TCP_DEBUG
 .equiv _KERNEL_OPT_TCP_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_TCP_DEBUG\n .global _KERNEL_OPT_TCP_DEBUG\n .equiv _KERNEL_OPT_TCP_DEBUG,0x6e074def\n .endif");
#endif
/* option `TCP_NDEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_TCP_NDEBUG
 .global _KERNEL_OPT_TCP_NDEBUG
 .equiv _KERNEL_OPT_TCP_NDEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_TCP_NDEBUG\n .global _KERNEL_OPT_TCP_NDEBUG\n .equiv _KERNEL_OPT_TCP_NDEBUG,0x6e074def\n .endif");
#endif
