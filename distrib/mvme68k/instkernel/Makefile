#	$NetBSD: Makefile,v 1.6 2002/04/26 15:50:39 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	RAMDISK ${RAMDISK} -

MDSETDIR=	installation/tapeimage

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
