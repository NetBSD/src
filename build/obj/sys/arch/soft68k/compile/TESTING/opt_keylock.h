/* option `KEYLOCK' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KEYLOCK
 .global _KERNEL_OPT_KEYLOCK
 .equiv _KERNEL_OPT_KEYLOCK,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KEYLOCK\n .global _KERNEL_OPT_KEYLOCK\n .equiv _KERNEL_OPT_KEYLOCK,0x6e074def\n .endif");
#endif
