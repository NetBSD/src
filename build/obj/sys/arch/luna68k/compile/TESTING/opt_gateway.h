/* option `GATEWAY' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_GATEWAY
 .global _KERNEL_OPT_GATEWAY
 .equiv _KERNEL_OPT_GATEWAY,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_GATEWAY\n .global _KERNEL_OPT_GATEWAY\n .equiv _KERNEL_OPT_GATEWAY,0x6e074def\n .endif");
#endif
