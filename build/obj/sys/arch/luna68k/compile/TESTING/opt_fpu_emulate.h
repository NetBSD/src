/* option `FPU_EMULATE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_FPU_EMULATE
 .global _KERNEL_OPT_FPU_EMULATE
 .equiv _KERNEL_OPT_FPU_EMULATE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_FPU_EMULATE\n .global _KERNEL_OPT_FPU_EMULATE\n .equiv _KERNEL_OPT_FPU_EMULATE,0x6e074def\n .endif");
#endif
