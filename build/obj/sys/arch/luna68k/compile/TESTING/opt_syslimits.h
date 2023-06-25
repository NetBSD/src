/* option `OPEN_MAX' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_OPEN_MAX
 .global _KERNEL_OPT_OPEN_MAX
 .equiv _KERNEL_OPT_OPEN_MAX,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_OPEN_MAX\n .global _KERNEL_OPT_OPEN_MAX\n .equiv _KERNEL_OPT_OPEN_MAX,0x6e074def\n .endif");
#endif
/* option `CHILD_MAX' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CHILD_MAX
 .global _KERNEL_OPT_CHILD_MAX
 .equiv _KERNEL_OPT_CHILD_MAX,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CHILD_MAX\n .global _KERNEL_OPT_CHILD_MAX\n .equiv _KERNEL_OPT_CHILD_MAX,0x6e074def\n .endif");
#endif
