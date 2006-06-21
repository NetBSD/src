#	$NetBSD: Makefile,v 1.9 2006/06/21 09:25:27 garbled Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL		${RAMDISK}	-	\
			INSTALL_SMALL	${RAMDISK}	-
MDSET_RELEASEDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
