/* option `KUBSAN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KUBSAN
 .global _KERNEL_OPT_KUBSAN
 .equiv _KERNEL_OPT_KUBSAN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KUBSAN\n .global _KERNEL_OPT_KUBSAN\n .equiv _KERNEL_OPT_KUBSAN,0x6e074def\n .endif");
#endif
