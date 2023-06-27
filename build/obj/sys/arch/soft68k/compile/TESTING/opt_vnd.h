/* option `VND_COMPRESSION' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_VND_COMPRESSION
 .global _KERNEL_OPT_VND_COMPRESSION
 .equiv _KERNEL_OPT_VND_COMPRESSION,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_VND_COMPRESSION\n .global _KERNEL_OPT_VND_COMPRESSION\n .equiv _KERNEL_OPT_VND_COMPRESSION,0x6e074def\n .endif");
#endif
