#	$NetBSD: Makefile.inc,v 1.16.18.2 2007/07/24 10:49:28 pavel Exp $
#
#	etc.pmax/Makefile.inc -- pmax-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL RAMDISK

INSTALLATION_DIRS+=	installation/diskimage	\
			installation/netboot \
			installation/miniroot
