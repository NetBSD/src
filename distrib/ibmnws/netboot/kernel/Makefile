#	$NetBSD: Makefile,v 1.1 2003/10/19 03:37:16 matt Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

NCDCS = ${TOOLDIR}/bin/${_TOOL_PREFIX}ibmnws-ncdcs

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		GENERIC		${RAMDISK}	-
MDSET_RELEASEDIR=	binary/kernel

MDSET_POST.- = \
	${NCDCS} ${.TARGET} ${.TARGET}.1000

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
