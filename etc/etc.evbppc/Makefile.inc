#	$NetBSD: Makefile.inc,v 1.6.30.1 2011/01/07 03:01:07 matt Exp $
#
#	etc.evbppc/Makefile.inc -- evbppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		WALNUT EV64260 EXPLORA451 OPENBLOCKS200 OPENBLOCKS266
KERNEL_SETS+=		MPC8536DS MPC8548CDS RB800
KERNEL_SUFFIXES=	img

BUILD_KERNELS=		INSTALL_WALNUT INSTALL_OPENBLOCKS266
