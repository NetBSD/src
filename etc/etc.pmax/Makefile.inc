#	$NetBSD: Makefile.inc,v 1.13.4.1 2005/04/06 14:41:16 he Exp $
#
#	etc.pmax/Makefile.inc -- pmax-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC WSCONS

BUILD_KERNELS=		INSTALL RAMDISK

INSTALLATION_DIRS+=	installation/diskimage	\
			installation/netboot \
			installation/miniroot
