#	$NetBSD: Makefile.inc,v 1.14 2002/12/02 13:18:13 lukem Exp $
#
#	etc.sun3/Makefile.inc -- sun3-specific etc Makefile targets
#

BIN3+= etc.sun3/ttyaction

KERNEL_SETS=		GENERIC GENERIC3X DISKLESS DISKLESS3X

EXTRA_KERNELS=		INSTALL INSTALL3X

BUILD_KERNELS=		RAMDISK RAMDISK3X

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/netboot	\
			installation/tapeimage
