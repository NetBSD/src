#	$NetBSD: Makefile.inc,v 1.13 2003/12/13 23:11:17 ad Exp $
#
#	etc.pmax/Makefile.inc -- pmax-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC WSCONS

BUILD_KERNELS=		INSTALL RAMDISK

INSTALLATION_DIRS+=	installation/diskimage	\
			installation/netboot
