/* option `MROUTING' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_MROUTING
 .global _KERNEL_OPT_MROUTING
 .equiv _KERNEL_OPT_MROUTING,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_MROUTING\n .global _KERNEL_OPT_MROUTING\n .equiv _KERNEL_OPT_MROUTING,0x6e074def\n .endif");
#endif
