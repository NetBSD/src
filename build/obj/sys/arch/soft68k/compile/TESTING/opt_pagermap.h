/* option `PAGER_MAP_SIZE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PAGER_MAP_SIZE
 .global _KERNEL_OPT_PAGER_MAP_SIZE
 .equiv _KERNEL_OPT_PAGER_MAP_SIZE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PAGER_MAP_SIZE\n .global _KERNEL_OPT_PAGER_MAP_SIZE\n .equiv _KERNEL_OPT_PAGER_MAP_SIZE,0x6e074def\n .endif");
#endif
