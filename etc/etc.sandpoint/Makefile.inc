#	$NetBSD: Makefile.inc,v 1.11 2009/08/24 14:17:05 nisimura Exp $
#
#	etc.sandpoint/Makefile.inc -- sandpoint-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERIC.NAS
KERNEL_SUFFIXES=	srec

BUILD_KERNELS=  	INSTALL
