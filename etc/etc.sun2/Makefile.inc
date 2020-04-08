#	$NetBSD: Makefile.inc,v 1.7.80.1 2020/04/08 14:03:57 martin Exp $
#
#	etc.sun2/Makefile.inc -- sun2-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC FOURMEG DISKLESS NFS

EXTRA_KERNELS=		INSTALL

BUILD_KERNELS=		RAMDISK

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/netboot	\
			installation/tapeimage
