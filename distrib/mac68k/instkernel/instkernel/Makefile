#	$NetBSD: Makefile,v 1.14 2002/04/26 15:50:33 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	INSTALL		${RAMDISK}	netbsd-INSTALL		\
		INSTALLSBC	${RAMDISK}	netbsd-INSTALLSBC

MDSETDIR=	installation/instkernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
