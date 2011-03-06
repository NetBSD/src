#	$NetBSD: Makefile.inc,v 1.1.2.2 2011/03/06 15:07:38 bouyer Exp $
#
#	etc.emips/Makefile.inc -- emips-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL RAMDISK

INSTALLATION_DIRS+=	installation/diskimage	\
			installation/netboot \
			installation/miniroot
