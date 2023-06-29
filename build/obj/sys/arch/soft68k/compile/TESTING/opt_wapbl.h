/* option `WAPBL_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_WAPBL_DEBUG
 .global _KERNEL_OPT_WAPBL_DEBUG
 .equiv _KERNEL_OPT_WAPBL_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_WAPBL_DEBUG\n .global _KERNEL_OPT_WAPBL_DEBUG\n .equiv _KERNEL_OPT_WAPBL_DEBUG,0x6e074def\n .endif");
#endif
#define	WAPBL	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_WAPBL
 .global _KERNEL_OPT_WAPBL
 .equiv _KERNEL_OPT_WAPBL,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_WAPBL\n .global _KERNEL_OPT_WAPBL\n .equiv _KERNEL_OPT_WAPBL,0x1\n .endif");
#endif
/* option `WAPBL_DEBUG_PRINT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_WAPBL_DEBUG_PRINT
 .global _KERNEL_OPT_WAPBL_DEBUG_PRINT
 .equiv _KERNEL_OPT_WAPBL_DEBUG_PRINT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_WAPBL_DEBUG_PRINT\n .global _KERNEL_OPT_WAPBL_DEBUG_PRINT\n .equiv _KERNEL_OPT_WAPBL_DEBUG_PRINT,0x6e074def\n .endif");
#endif
