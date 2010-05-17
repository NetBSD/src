#	$NetBSD: Makefile.inc,v 1.12 2010/05/17 22:52:17 phx Exp $
#
#	etc.sandpoint/Makefile.inc -- sandpoint-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERIC.NAS KUROBOX
KERNEL_SUFFIXES=	srec

BUILD_KERNELS=  	INSTALL
