#	$NetBSD: Makefile,v 1.20 2002/04/11 15:34:52 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
TARGETS=	INSTALL	${RAMDISK}

MDSETDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"
