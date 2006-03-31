#	$NetBSD: Makefile.inc,v 1.8 2006/03/31 21:25:59 he Exp $
#
#	etc.evbmips/Makefile.inc -- evbmips-specific etc Makefile targets
#

KERNEL_SETS=		ALCHEMY AR531X DBAU1500 DBAU1550 MALTA MTX-1 OMSAL400
KERNEL_SUFFIXES=	srec

.if ${MACHINE_ARCH} == "mipsel"
BUILD_KERNELS=		INSTALL_OMSAL400
.endif
