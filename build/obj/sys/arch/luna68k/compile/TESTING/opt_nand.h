/* option `NAND_BBT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NAND_BBT
 .global _KERNEL_OPT_NAND_BBT
 .equiv _KERNEL_OPT_NAND_BBT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NAND_BBT\n .global _KERNEL_OPT_NAND_BBT\n .equiv _KERNEL_OPT_NAND_BBT,0x6e074def\n .endif");
#endif
/* option `NAND_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NAND_DEBUG
 .global _KERNEL_OPT_NAND_DEBUG
 .equiv _KERNEL_OPT_NAND_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NAND_DEBUG\n .global _KERNEL_OPT_NAND_DEBUG\n .equiv _KERNEL_OPT_NAND_DEBUG,0x6e074def\n .endif");
#endif
/* option `NAND_VERBOSE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NAND_VERBOSE
 .global _KERNEL_OPT_NAND_VERBOSE
 .equiv _KERNEL_OPT_NAND_VERBOSE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NAND_VERBOSE\n .global _KERNEL_OPT_NAND_VERBOSE\n .equiv _KERNEL_OPT_NAND_VERBOSE,0x6e074def\n .endif");
#endif
