/* option `CFI_DEBUG_JEDEC' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CFI_DEBUG_JEDEC
 .global _KERNEL_OPT_CFI_DEBUG_JEDEC
 .equiv _KERNEL_OPT_CFI_DEBUG_JEDEC,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CFI_DEBUG_JEDEC\n .global _KERNEL_OPT_CFI_DEBUG_JEDEC\n .equiv _KERNEL_OPT_CFI_DEBUG_JEDEC,0x6e074def\n .endif");
#endif
/* option `CFI_DEBUG_QRY' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CFI_DEBUG_QRY
 .global _KERNEL_OPT_CFI_DEBUG_QRY
 .equiv _KERNEL_OPT_CFI_DEBUG_QRY,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CFI_DEBUG_QRY\n .global _KERNEL_OPT_CFI_DEBUG_QRY\n .equiv _KERNEL_OPT_CFI_DEBUG_QRY,0x6e074def\n .endif");
#endif
