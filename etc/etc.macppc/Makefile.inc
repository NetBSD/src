#	$NetBSD: Makefile.inc,v 1.7 2002/12/02 13:31:19 lukem Exp $
#
#	etc.macppc/Makefile.inc -- macppc-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC.MP

BUILD_KERNELS+=		INSTALL GENERIC_MD

INSTALLATION_DIRS+=	installation/floppy
