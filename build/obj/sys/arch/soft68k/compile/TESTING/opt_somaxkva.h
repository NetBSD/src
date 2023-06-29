/* option `SOMAXKVA' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SOMAXKVA
 .global _KERNEL_OPT_SOMAXKVA
 .equiv _KERNEL_OPT_SOMAXKVA,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SOMAXKVA\n .global _KERNEL_OPT_SOMAXKVA\n .equiv _KERNEL_OPT_SOMAXKVA,0x6e074def\n .endif");
#endif
