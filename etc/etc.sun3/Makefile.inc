#	$NetBSD: Makefile.inc,v 1.17.18.2 2007/07/24 10:49:29 pavel Exp $
#
#	etc.sun3/Makefile.inc -- sun3-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERIC3X DISKLESS DISKLESS3X

EXTRA_KERNELS=		INSTALL INSTALL3X

BUILD_KERNELS=		RAMDISK RAMDISK3X

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/netboot	\
			installation/tapeimage
