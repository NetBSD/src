#	$NetBSD: Makefile.inc,v 1.7.4.1 2011/06/23 14:18:26 cherry Exp $
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
