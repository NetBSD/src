#	$NetBSD: Makefile.inc,v 1.4 2002/12/02 13:18:13 lukem Exp $
#
#	etc.sun2/Makefile.inc -- sun2-specific etc Makefile targets
#

BIN3+= etc.sun2/ttyaction

KERNEL_SETS=		GENERIC FOURMEG DISKLESS

EXTRA_KERNELS=		INSTALL

BUILD_KERNELS=		RAMDISK

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/netboot	\
			installation/tapeimage
