/* option `CCITT' is obsolete */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CCITT
 .global _KERNEL_OPT_CCITT
 .equiv _KERNEL_OPT_CCITT,0xdeadbeef
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CCITT\n .global _KERNEL_OPT_CCITT\n .equiv _KERNEL_OPT_CCITT,0xdeadbeef\n .endif");
#endif
