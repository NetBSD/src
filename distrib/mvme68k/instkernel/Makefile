#	$NetBSD: Makefile,v 1.5 2002/04/12 13:39:47 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	RAMDISK ${RAMDISK} -

MDSETDIR=	installation/tapeimage

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
