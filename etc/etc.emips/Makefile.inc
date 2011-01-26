#	$NetBSD: Makefile.inc,v 1.1 2011/01/26 01:18:47 pooka Exp $
#
#	etc.emips/Makefile.inc -- emips-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL RAMDISK

INSTALLATION_DIRS+=	installation/diskimage	\
			installation/netboot \
			installation/miniroot
