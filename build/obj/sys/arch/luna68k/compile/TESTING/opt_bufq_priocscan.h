/* option `BUFQ_PRIOCSCAN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_BUFQ_PRIOCSCAN
 .global _KERNEL_OPT_BUFQ_PRIOCSCAN
 .equiv _KERNEL_OPT_BUFQ_PRIOCSCAN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_BUFQ_PRIOCSCAN\n .global _KERNEL_OPT_BUFQ_PRIOCSCAN\n .equiv _KERNEL_OPT_BUFQ_PRIOCSCAN,0x6e074def\n .endif");
#endif
