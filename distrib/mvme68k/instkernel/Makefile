#	$NetBSD: Makefile,v 1.8 2002/05/08 21:44:57 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		RAMDISK ${RAMDISK} -
MDSET_RELEASEDIR=	installation/tapeimage

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
