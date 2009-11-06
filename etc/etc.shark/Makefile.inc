#	$NetBSD: Makefile.inc,v 1.11 2009/11/06 12:31:44 is Exp $
#
#	etc.shark/Makefile.inc -- shark-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC
KERNEL_SUFFIXES=	aout

BUILD_KERNELS=		INSTALL
