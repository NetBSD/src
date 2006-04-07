#	$NetBSD: Makefile.inc,v 1.9 2006/04/07 05:08:04 gdamore Exp $
#
#	etc.evbmips/Makefile.inc -- evbmips-specific etc Makefile targets
#

KERNEL_SETS=		ALCHEMY DBAU1500 DBAU1550 MALTA MTX-1 OMSAL400
KERNEL_SUFFIXES=	srec

.if ${MACHINE_ARCH} == "mipsel"
BUILD_KERNELS=		INSTALL_OMSAL400
.endif
