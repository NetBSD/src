#	$NetBSD: Makefile.inc,v 1.13 2010/06/09 17:34:32 phx Exp $
#
#	etc.sandpoint/Makefile.inc -- sandpoint-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC KUROBOX
KERNEL_SUFFIXES=	srec

BUILD_KERNELS=  	INSTALL INSTALL_KURO
