/* option `SB_MAX' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SB_MAX
 .global _KERNEL_OPT_SB_MAX
 .equiv _KERNEL_OPT_SB_MAX,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SB_MAX\n .global _KERNEL_OPT_SB_MAX\n .equiv _KERNEL_OPT_SB_MAX,0x6e074def\n .endif");
#endif
