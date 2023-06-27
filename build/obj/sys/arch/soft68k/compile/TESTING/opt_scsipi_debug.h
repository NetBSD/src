/* option `SCSIPI_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SCSIPI_DEBUG
 .global _KERNEL_OPT_SCSIPI_DEBUG
 .equiv _KERNEL_OPT_SCSIPI_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SCSIPI_DEBUG\n .global _KERNEL_OPT_SCSIPI_DEBUG\n .equiv _KERNEL_OPT_SCSIPI_DEBUG,0x6e074def\n .endif");
#endif
/* option `SCSIPI_DEBUG_FLAGS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SCSIPI_DEBUG_FLAGS
 .global _KERNEL_OPT_SCSIPI_DEBUG_FLAGS
 .equiv _KERNEL_OPT_SCSIPI_DEBUG_FLAGS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SCSIPI_DEBUG_FLAGS\n .global _KERNEL_OPT_SCSIPI_DEBUG_FLAGS\n .equiv _KERNEL_OPT_SCSIPI_DEBUG_FLAGS,0x6e074def\n .endif");
#endif
/* option `SCSIPI_DEBUG_LUN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SCSIPI_DEBUG_LUN
 .global _KERNEL_OPT_SCSIPI_DEBUG_LUN
 .equiv _KERNEL_OPT_SCSIPI_DEBUG_LUN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SCSIPI_DEBUG_LUN\n .global _KERNEL_OPT_SCSIPI_DEBUG_LUN\n .equiv _KERNEL_OPT_SCSIPI_DEBUG_LUN,0x6e074def\n .endif");
#endif
/* option `SCSIPI_DEBUG_TARGET' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SCSIPI_DEBUG_TARGET
 .global _KERNEL_OPT_SCSIPI_DEBUG_TARGET
 .equiv _KERNEL_OPT_SCSIPI_DEBUG_TARGET,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SCSIPI_DEBUG_TARGET\n .global _KERNEL_OPT_SCSIPI_DEBUG_TARGET\n .equiv _KERNEL_OPT_SCSIPI_DEBUG_TARGET,0x6e074def\n .endif");
#endif
/* option `SCSIPI_DEBUG_TYPE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SCSIPI_DEBUG_TYPE
 .global _KERNEL_OPT_SCSIPI_DEBUG_TYPE
 .equiv _KERNEL_OPT_SCSIPI_DEBUG_TYPE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SCSIPI_DEBUG_TYPE\n .global _KERNEL_OPT_SCSIPI_DEBUG_TYPE\n .equiv _KERNEL_OPT_SCSIPI_DEBUG_TYPE,0x6e074def\n .endif");
#endif
