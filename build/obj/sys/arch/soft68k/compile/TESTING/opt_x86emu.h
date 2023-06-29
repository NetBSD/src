/* option `X86EMU' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_X86EMU
 .global _KERNEL_OPT_X86EMU
 .equiv _KERNEL_OPT_X86EMU,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_X86EMU\n .global _KERNEL_OPT_X86EMU\n .equiv _KERNEL_OPT_X86EMU,0x6e074def\n .endif");
#endif
