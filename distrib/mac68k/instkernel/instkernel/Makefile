#	$NetBSD: Makefile,v 1.13 2002/04/12 13:39:47 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	INSTALL		${RAMDISK}	netbsd-INSTALL		\
		INSTALLSBC	${RAMDISK}	netbsd-INSTALLSBC

MDSETDIR=	installation/instkernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
