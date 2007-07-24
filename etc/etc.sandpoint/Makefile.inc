#	$NetBSD: Makefile.inc,v 1.9 2007/07/24 10:49:27 pavel Exp $
#
#	etc.sandpoint/Makefile.inc -- sandpoint-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERIC.X2
KERNEL_SUFFIXES=	srec

BUILD_KERNELS=  	INSTALL
