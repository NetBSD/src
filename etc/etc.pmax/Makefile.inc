#	$NetBSD: Makefile.inc,v 1.14 2004/12/15 15:03:14 jmc Exp $
#
#	etc.pmax/Makefile.inc -- pmax-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC WSCONS

BUILD_KERNELS=		INSTALL RAMDISK

INSTALLATION_DIRS+=	installation/diskimage	\
			installation/netboot \
			installation/miniroot
