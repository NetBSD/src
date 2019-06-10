#	$NetBSD: Makefile,v 1.11.102.1 2019/06/10 21:42:25 christos Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL		${RAMDISK}	-	\
			INSTALL_601	${RAMDISK}	-	\
			GENERIC_MD	${RAMDISK}	-
MDSET_RELEASEDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
