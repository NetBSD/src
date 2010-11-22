#	$NetBSD: Makefile.inc,v 1.10.6.1 2010/11/22 03:15:33 riz Exp $
#
#	etc.shark/Makefile.inc -- shark-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC
KERNEL_SUFFIXES=	aout

BUILD_KERNELS=		INSTALL
