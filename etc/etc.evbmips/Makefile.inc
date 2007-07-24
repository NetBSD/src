#	$NetBSD: Makefile.inc,v 1.11 2007/07/24 10:49:25 pavel Exp $
#
#	etc.evbmips/Makefile.inc -- evbmips-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SUFFIXES=	srec

.if ${MACHINE_ARCH} == "mipsel"
KERNEL_SETS=		ALCHEMY DBAU1500 DBAU1550 MALTA MTX-1 OMSAL400
BUILD_KERNELS=		INSTALL_OMSAL400

.else
KERNEL_SETS=		ALCHEMY DBAU1500 DBAU1550 MALTA

.endif

