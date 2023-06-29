/* option `MPLS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_MPLS
 .global _KERNEL_OPT_MPLS
 .equiv _KERNEL_OPT_MPLS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_MPLS\n .global _KERNEL_OPT_MPLS\n .equiv _KERNEL_OPT_MPLS,0x6e074def\n .endif");
#endif
