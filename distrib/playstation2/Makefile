#	$NetBSD: Makefile,v 1.8 2002/04/12 13:39:48 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

MINIROOTDIR!=	cd ${.CURDIR}/../miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTDIR}/ramdisk.fs
MDSETTARGETS=	RAMDISK ${MINIROOT} netbsd

MDSETDIR=	installation

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
