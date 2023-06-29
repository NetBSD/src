/* option `BIOHIST' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_BIOHIST
 .global _KERNEL_OPT_BIOHIST
 .equiv _KERNEL_OPT_BIOHIST,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_BIOHIST\n .global _KERNEL_OPT_BIOHIST\n .equiv _KERNEL_OPT_BIOHIST,0x6e074def\n .endif");
#endif
/* option `BIOHIST_PRINT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_BIOHIST_PRINT
 .global _KERNEL_OPT_BIOHIST_PRINT
 .equiv _KERNEL_OPT_BIOHIST_PRINT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_BIOHIST_PRINT\n .global _KERNEL_OPT_BIOHIST_PRINT\n .equiv _KERNEL_OPT_BIOHIST_PRINT,0x6e074def\n .endif");
#endif
/* option `BIOHIST_SIZE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_BIOHIST_SIZE
 .global _KERNEL_OPT_BIOHIST_SIZE
 .equiv _KERNEL_OPT_BIOHIST_SIZE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_BIOHIST_SIZE\n .global _KERNEL_OPT_BIOHIST_SIZE\n .equiv _KERNEL_OPT_BIOHIST_SIZE,0x6e074def\n .endif");
#endif
