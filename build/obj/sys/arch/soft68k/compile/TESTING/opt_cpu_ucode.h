/* option `CPU_UCODE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CPU_UCODE
 .global _KERNEL_OPT_CPU_UCODE
 .equiv _KERNEL_OPT_CPU_UCODE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CPU_UCODE\n .global _KERNEL_OPT_CPU_UCODE\n .equiv _KERNEL_OPT_CPU_UCODE,0x6e074def\n .endif");
#endif
