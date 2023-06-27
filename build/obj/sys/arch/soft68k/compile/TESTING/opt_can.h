/* option `CAN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CAN
 .global _KERNEL_OPT_CAN
 .equiv _KERNEL_OPT_CAN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CAN\n .global _KERNEL_OPT_CAN\n .equiv _KERNEL_OPT_CAN,0x6e074def\n .endif");
#endif
