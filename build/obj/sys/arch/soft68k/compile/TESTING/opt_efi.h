/* option `EFI_RUNTIME' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_EFI_RUNTIME
 .global _KERNEL_OPT_EFI_RUNTIME
 .equiv _KERNEL_OPT_EFI_RUNTIME,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_EFI_RUNTIME\n .global _KERNEL_OPT_EFI_RUNTIME\n .equiv _KERNEL_OPT_EFI_RUNTIME,0x6e074def\n .endif");
#endif
