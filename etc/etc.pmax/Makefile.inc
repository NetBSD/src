#	$NetBSD: Makefile.inc,v 1.12 2002/12/02 13:18:11 lukem Exp $
#
#	etc.pmax/Makefile.inc -- pmax-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL RAMDISK

INSTALLATION_DIRS+=	installation/diskimage	\
			installation/netboot
