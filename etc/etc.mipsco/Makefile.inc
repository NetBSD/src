#	$NetBSD: Makefile.inc,v 1.2 2002/12/02 13:31:00 lukem Exp $
#
#	etc.mipsco/Makefile.inc -- mipsco-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/diskimage	\
			installation/netboot
