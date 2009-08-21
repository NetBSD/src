#	$NetBSD: Makefile.inc,v 1.7 2009/08/21 16:57:08 tsutsui Exp $
#
#	etc.evbppc/Makefile.inc -- evbppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		WALNUT EV64260 EXPLORA451 OPENBLOCKS200 OPENBLOCKS266
KERNEL_SETS+=		PMPPC
KERNEL_SUFFIXES=	img

BUILD_KERNELS=		INSTALL_WALNUT INSTALL_OPENBLOCKS266
