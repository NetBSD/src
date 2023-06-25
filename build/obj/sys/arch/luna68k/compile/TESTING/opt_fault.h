/* option `FAULT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_FAULT
 .global _KERNEL_OPT_FAULT
 .equiv _KERNEL_OPT_FAULT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_FAULT\n .global _KERNEL_OPT_FAULT\n .equiv _KERNEL_OPT_FAULT,0x6e074def\n .endif");
#endif
