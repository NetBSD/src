#	$NetBSD: Makefile.inc,v 1.16 2002/12/17 15:55:01 jdolecek Exp $
#
#	etc.sun3/Makefile.inc -- sun3-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC3X DISKLESS DISKLESS3X

EXTRA_KERNELS=		INSTALL INSTALL3X

BUILD_KERNELS=		RAMDISK RAMDISK3X

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/netboot	\
			installation/tapeimage
