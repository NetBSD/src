/* option `LLC' is obsolete */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_LLC
 .global _KERNEL_OPT_LLC
 .equiv _KERNEL_OPT_LLC,0xdeadbeef
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_LLC\n .global _KERNEL_OPT_LLC\n .equiv _KERNEL_OPT_LLC,0xdeadbeef\n .endif");
#endif
