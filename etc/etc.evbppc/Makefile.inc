#	$NetBSD: Makefile.inc,v 1.6.18.2 2007/07/24 10:49:26 pavel Exp $
#
#	etc.evbppc/Makefile.inc -- evbppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		WALNUT EV64260 EXPLORA451 OPENBLOCKS200 OPENBLOCKS266
KERNEL_SUFFIXES=	img

BUILD_KERNELS=		INSTALL_WALNUT INSTALL_OPENBLOCKS266
