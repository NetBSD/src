#	$NetBSD: Makefile,v 1.6 2002/04/11 15:34:59 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

MINIROOTDIR!=	cd ${.CURDIR}/../miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTDIR}/ramdisk.fs
TARGETS=	RAMDISK	${MINIROOT}

MDSETDIR=	installation

.include "${DISTRIBDIR}/common/Makefile.mdset"
