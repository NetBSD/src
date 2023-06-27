/* option `SCSI_OLD_NOINQUIRY' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SCSI_OLD_NOINQUIRY
 .global _KERNEL_OPT_SCSI_OLD_NOINQUIRY
 .equiv _KERNEL_OPT_SCSI_OLD_NOINQUIRY,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SCSI_OLD_NOINQUIRY\n .global _KERNEL_OPT_SCSI_OLD_NOINQUIRY\n .equiv _KERNEL_OPT_SCSI_OLD_NOINQUIRY,0x6e074def\n .endif");
#endif
/* option `SES_ENABLE_PASSTHROUGH' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SES_ENABLE_PASSTHROUGH
 .global _KERNEL_OPT_SES_ENABLE_PASSTHROUGH
 .equiv _KERNEL_OPT_SES_ENABLE_PASSTHROUGH,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SES_ENABLE_PASSTHROUGH\n .global _KERNEL_OPT_SES_ENABLE_PASSTHROUGH\n .equiv _KERNEL_OPT_SES_ENABLE_PASSTHROUGH,0x6e074def\n .endif");
#endif
/* option `ST_SUNCOMPAT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_ST_SUNCOMPAT
 .global _KERNEL_OPT_ST_SUNCOMPAT
 .equiv _KERNEL_OPT_ST_SUNCOMPAT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_ST_SUNCOMPAT\n .global _KERNEL_OPT_ST_SUNCOMPAT\n .equiv _KERNEL_OPT_ST_SUNCOMPAT,0x6e074def\n .endif");
#endif
/* option `ST_ENABLE_EARLYWARN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_ST_ENABLE_EARLYWARN
 .global _KERNEL_OPT_ST_ENABLE_EARLYWARN
 .equiv _KERNEL_OPT_ST_ENABLE_EARLYWARN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_ST_ENABLE_EARLYWARN\n .global _KERNEL_OPT_ST_ENABLE_EARLYWARN\n .equiv _KERNEL_OPT_ST_ENABLE_EARLYWARN,0x6e074def\n .endif");
#endif
/* option `SCSIVERBOSE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SCSIVERBOSE
 .global _KERNEL_OPT_SCSIVERBOSE
 .equiv _KERNEL_OPT_SCSIVERBOSE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SCSIVERBOSE\n .global _KERNEL_OPT_SCSIVERBOSE\n .equiv _KERNEL_OPT_SCSIVERBOSE,0x6e074def\n .endif");
#endif
/* option `SD_IO_TIMEOUT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SD_IO_TIMEOUT
 .global _KERNEL_OPT_SD_IO_TIMEOUT
 .equiv _KERNEL_OPT_SD_IO_TIMEOUT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SD_IO_TIMEOUT\n .global _KERNEL_OPT_SD_IO_TIMEOUT\n .equiv _KERNEL_OPT_SD_IO_TIMEOUT,0x6e074def\n .endif");
#endif
/* option `SDRETRIES' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_SDRETRIES
 .global _KERNEL_OPT_SDRETRIES
 .equiv _KERNEL_OPT_SDRETRIES,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_SDRETRIES\n .global _KERNEL_OPT_SDRETRIES\n .equiv _KERNEL_OPT_SDRETRIES,0x6e074def\n .endif");
#endif
/* option `ST_MOUNT_DELAY' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_ST_MOUNT_DELAY
 .global _KERNEL_OPT_ST_MOUNT_DELAY
 .equiv _KERNEL_OPT_ST_MOUNT_DELAY,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_ST_MOUNT_DELAY\n .global _KERNEL_OPT_ST_MOUNT_DELAY\n .equiv _KERNEL_OPT_ST_MOUNT_DELAY,0x6e074def\n .endif");
#endif
