#	$NetBSD: Makefile.inc,v 1.15 2006/07/29 19:10:57 ad Exp $
#
#	etc.pmax/Makefile.inc -- pmax-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL RAMDISK

INSTALLATION_DIRS+=	installation/diskimage	\
			installation/netboot \
			installation/miniroot
