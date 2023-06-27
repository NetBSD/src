/* option `secmodel_keylock' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_secmodel_keylock
 .global _KERNEL_OPT_secmodel_keylock
 .equiv _KERNEL_OPT_secmodel_keylock,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_secmodel_keylock\n .global _KERNEL_OPT_secmodel_keylock\n .equiv _KERNEL_OPT_secmodel_keylock,0x6e074def\n .endif");
#endif
