/* option `FDTBASE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_FDTBASE
 .global _KERNEL_OPT_FDTBASE
 .equiv _KERNEL_OPT_FDTBASE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_FDTBASE\n .global _KERNEL_OPT_FDTBASE\n .equiv _KERNEL_OPT_FDTBASE,0x6e074def\n .endif");
#endif
/* option `FDT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_FDT
 .global _KERNEL_OPT_FDT
 .equiv _KERNEL_OPT_FDT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_FDT\n .global _KERNEL_OPT_FDT\n .equiv _KERNEL_OPT_FDT,0x6e074def\n .endif");
#endif
/* option `FDT_MEMORY_RANGES' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_FDT_MEMORY_RANGES
 .global _KERNEL_OPT_FDT_MEMORY_RANGES
 .equiv _KERNEL_OPT_FDT_MEMORY_RANGES,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_FDT_MEMORY_RANGES\n .global _KERNEL_OPT_FDT_MEMORY_RANGES\n .equiv _KERNEL_OPT_FDT_MEMORY_RANGES,0x6e074def\n .endif");
#endif
/* option `FDT_DEFAULT_STDOUT_PATH' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_FDT_DEFAULT_STDOUT_PATH
 .global _KERNEL_OPT_FDT_DEFAULT_STDOUT_PATH
 .equiv _KERNEL_OPT_FDT_DEFAULT_STDOUT_PATH,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_FDT_DEFAULT_STDOUT_PATH\n .global _KERNEL_OPT_FDT_DEFAULT_STDOUT_PATH\n .equiv _KERNEL_OPT_FDT_DEFAULT_STDOUT_PATH,0x6e074def\n .endif");
#endif
