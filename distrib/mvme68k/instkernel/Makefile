#	$NetBSD: Makefile,v 1.3 2002/04/11 15:34:57 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
TARGETS=	RAMDISK ${RAMDISK}

MDSETDIR=	installation/tapeimage

.include "${DISTRIBDIR}/common/Makefile.mdset"
