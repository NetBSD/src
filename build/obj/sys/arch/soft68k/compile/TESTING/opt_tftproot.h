/* option `TFTPROOT_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_TFTPROOT_DEBUG
 .global _KERNEL_OPT_TFTPROOT_DEBUG
 .equiv _KERNEL_OPT_TFTPROOT_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_TFTPROOT_DEBUG\n .global _KERNEL_OPT_TFTPROOT_DEBUG\n .equiv _KERNEL_OPT_TFTPROOT_DEBUG,0x6e074def\n .endif");
#endif
/* option `TFTPROOT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_TFTPROOT
 .global _KERNEL_OPT_TFTPROOT
 .equiv _KERNEL_OPT_TFTPROOT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_TFTPROOT\n .global _KERNEL_OPT_TFTPROOT\n .equiv _KERNEL_OPT_TFTPROOT,0x6e074def\n .endif");
#endif
