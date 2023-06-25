/* option `UVMHIST' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_UVMHIST
 .global _KERNEL_OPT_UVMHIST
 .equiv _KERNEL_OPT_UVMHIST,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_UVMHIST\n .global _KERNEL_OPT_UVMHIST\n .equiv _KERNEL_OPT_UVMHIST,0x6e074def\n .endif");
#endif
/* option `UVMHIST_PRINT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_UVMHIST_PRINT
 .global _KERNEL_OPT_UVMHIST_PRINT
 .equiv _KERNEL_OPT_UVMHIST_PRINT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_UVMHIST_PRINT\n .global _KERNEL_OPT_UVMHIST_PRINT\n .equiv _KERNEL_OPT_UVMHIST_PRINT,0x6e074def\n .endif");
#endif
/* option `UVMHIST_PDHIST_SIZE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_UVMHIST_PDHIST_SIZE
 .global _KERNEL_OPT_UVMHIST_PDHIST_SIZE
 .equiv _KERNEL_OPT_UVMHIST_PDHIST_SIZE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_UVMHIST_PDHIST_SIZE\n .global _KERNEL_OPT_UVMHIST_PDHIST_SIZE\n .equiv _KERNEL_OPT_UVMHIST_PDHIST_SIZE,0x6e074def\n .endif");
#endif
/* option `UVMHIST_MAPHIST_SIZE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_UVMHIST_MAPHIST_SIZE
 .global _KERNEL_OPT_UVMHIST_MAPHIST_SIZE
 .equiv _KERNEL_OPT_UVMHIST_MAPHIST_SIZE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_UVMHIST_MAPHIST_SIZE\n .global _KERNEL_OPT_UVMHIST_MAPHIST_SIZE\n .equiv _KERNEL_OPT_UVMHIST_MAPHIST_SIZE,0x6e074def\n .endif");
#endif
