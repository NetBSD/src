#	$NetBSD: Makefile.inc,v 1.8 2003/11/13 02:33:39 sekiya Exp $
#
#	etc.sgimips/Makefile.inc -- sgimips-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC32_IP2x GENERIC32_IP3x
KERNEL_SUFFIXES=	ecoff

#EXTRA_KERNELS=		NETBOOT

BUILD_KERNELS=		INSTALL32_IP2x INSTALL32_IP3x

INSTALLATION_DIRS+=	installation/netboot \
			installation/diskimage
