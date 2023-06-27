/* option `TCP_SENDSPACE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_TCP_SENDSPACE
 .global _KERNEL_OPT_TCP_SENDSPACE
 .equiv _KERNEL_OPT_TCP_SENDSPACE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_TCP_SENDSPACE\n .global _KERNEL_OPT_TCP_SENDSPACE\n .equiv _KERNEL_OPT_TCP_SENDSPACE,0x6e074def\n .endif");
#endif
/* option `TCP_RECVSPACE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_TCP_RECVSPACE
 .global _KERNEL_OPT_TCP_RECVSPACE
 .equiv _KERNEL_OPT_TCP_RECVSPACE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_TCP_RECVSPACE\n .global _KERNEL_OPT_TCP_RECVSPACE\n .equiv _KERNEL_OPT_TCP_RECVSPACE,0x6e074def\n .endif");
#endif
