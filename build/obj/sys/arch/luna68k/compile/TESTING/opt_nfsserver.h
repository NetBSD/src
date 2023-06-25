/* option `NFSSERVER' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFSSERVER
 .global _KERNEL_OPT_NFSSERVER
 .equiv _KERNEL_OPT_NFSSERVER,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFSSERVER\n .global _KERNEL_OPT_NFSSERVER\n .equiv _KERNEL_OPT_NFSSERVER,0x6e074def\n .endif");
#endif
