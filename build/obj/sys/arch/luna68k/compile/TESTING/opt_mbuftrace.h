/* option `MBUFTRACE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_MBUFTRACE
 .global _KERNEL_OPT_MBUFTRACE
 .equiv _KERNEL_OPT_MBUFTRACE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_MBUFTRACE\n .global _KERNEL_OPT_MBUFTRACE\n .equiv _KERNEL_OPT_MBUFTRACE,0x6e074def\n .endif");
#endif
