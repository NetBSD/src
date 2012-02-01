#	$NetBSD: Makefile.inc,v 1.9 2012/02/01 21:56:40 matt Exp $
#
#	etc.evbppc/Makefile.inc -- evbppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		WALNUT EXPLORA451 OPENBLOCKS200 OPENBLOCKS266
KERNEL_SETS+=		EV64260 PMPPC
KERNEL_SETS+=		MPC8536DS MPC8548CDS P2020DS P2020RDB RB800
KERNEL_SUFFIXES=	img

BUILD_KERNELS=		INSTALL_WALNUT INSTALL_OPENBLOCKS266 INSTALL_PMPPC
BUILD_KERNELS+=		INSTALL_P202DS INSTALL_P202RDB
