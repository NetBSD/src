#	$NetBSD: Makefile,v 1.7 2002/04/12 05:34:01 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

MINIROOTDIR!=	cd ${.CURDIR}/../miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTDIR}/ramdisk.fs
TARGETS=	RAMDISK ${MINIROOT}

MDSETDIR=	installation

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
