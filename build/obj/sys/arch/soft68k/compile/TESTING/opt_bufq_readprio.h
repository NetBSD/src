/* option `BUFQ_READPRIO' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_BUFQ_READPRIO
 .global _KERNEL_OPT_BUFQ_READPRIO
 .equiv _KERNEL_OPT_BUFQ_READPRIO,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_BUFQ_READPRIO\n .global _KERNEL_OPT_BUFQ_READPRIO\n .equiv _KERNEL_OPT_BUFQ_READPRIO,0x6e074def\n .endif");
#endif
