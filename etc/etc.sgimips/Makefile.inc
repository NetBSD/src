#	$NetBSD: Makefile.inc,v 1.7 2002/12/02 13:18:12 lukem Exp $
#
#	etc.sgimips/Makefile.inc -- sgimips-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC_INDY
KERNEL_SUFFIXES=	ecoff

#EXTRA_KERNELS=		NETBOOT

BUILD_KERNELS=		RAMDISK RAMDISK_INDY

INSTALLATION_DIRS+=	installation/netboot \
			installation/diskimage
