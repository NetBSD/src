#	$NetBSD: Makefile,v 1.6 2002/04/11 15:34:56 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
TARGETS=	INSTALL		${RAMDISK}	\
		GENERIC_MD	${RAMDISK}

MDSETDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"
