#	$NetBSD: Makefile,v 1.11 2002/05/08 21:44:55 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		RAMDISK ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
