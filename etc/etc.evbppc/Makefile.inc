#	$NetBSD: Makefile.inc,v 1.13 2015/01/30 15:04:35 nonaka Exp $
#
#	etc.evbppc/Makefile.inc -- evbppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

.if ${MACHINE_ARCH} == "powerpc"
KERNEL_SETS=		WALNUT EXPLORA451 OPENBLOCKS200 OPENBLOCKS266
KERNEL_SETS+=		EV64260 PMPPC
KERNEL_SETS+=		MPC8536DS MPC8548CDS P2020DS P2020RDB RB800 RB850GX2
KERNEL_SETS+=		TWRP1025
KERNEL_SUFFIXES=	img

BUILD_KERNELS=		INSTALL_WALNUT INSTALL_OPENBLOCKS266 INSTALL_PMPPC
BUILD_KERNELS+=		INSTALL_P2020DS INSTALL_P2020RDB INSTALL_RB850GX2
BUILD_KERNELS+=		INSTALL_TWRP1025
.endif
