#	$NetBSD: Makefile,v 1.9 2002/04/26 15:50:30 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	RAMDISK ${RAMDISK} -

MDSETDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
