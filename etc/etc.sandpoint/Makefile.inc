#	$NetBSD: Makefile.inc,v 1.9.4.1 2007/11/06 23:10:28 matt Exp $
#
#	etc.sandpoint/Makefile.inc -- sandpoint-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC
KERNEL_SUFFIXES=	srec

BUILD_KERNELS=  	INSTALL
