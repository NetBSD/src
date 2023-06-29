/* option `NET_MPSAFE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NET_MPSAFE
 .global _KERNEL_OPT_NET_MPSAFE
 .equiv _KERNEL_OPT_NET_MPSAFE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NET_MPSAFE\n .global _KERNEL_OPT_NET_MPSAFE\n .equiv _KERNEL_OPT_NET_MPSAFE,0x6e074def\n .endif");
#endif
