/* option `MIIVERBOSE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_MIIVERBOSE
 .global _KERNEL_OPT_MIIVERBOSE
 .equiv _KERNEL_OPT_MIIVERBOSE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_MIIVERBOSE\n .global _KERNEL_OPT_MIIVERBOSE\n .equiv _KERNEL_OPT_MIIVERBOSE,0x6e074def\n .endif");
#endif
