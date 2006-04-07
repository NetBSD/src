#	$NetBSD: Makefile.inc,v 1.10 2006/04/07 15:26:59 gdamore Exp $
#
#	etc.evbmips/Makefile.inc -- evbmips-specific etc Makefile targets
#

KERNEL_SUFFIXES=	srec

.if ${MACHINE_ARCH} == "mipsel"
KERNEL_SETS=		ALCHEMY DBAU1500 DBAU1550 MALTA MTX-1 OMSAL400
BUILD_KERNELS=		INSTALL_OMSAL400

.else
KERNEL_SETS=		ALCHEMY DBAU1500 DBAU1550 MALTA

.endif

