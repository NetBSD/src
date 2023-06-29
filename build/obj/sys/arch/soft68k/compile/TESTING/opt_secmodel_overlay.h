/* option `secmodel_overlay' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_secmodel_overlay
 .global _KERNEL_OPT_secmodel_overlay
 .equiv _KERNEL_OPT_secmodel_overlay,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_secmodel_overlay\n .global _KERNEL_OPT_secmodel_overlay\n .equiv _KERNEL_OPT_secmodel_overlay,0x6e074def\n .endif");
#endif
