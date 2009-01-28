#	$NetBSD: Makefile,v 1.2 2009/01/28 11:21:56 nonaka Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL ${RAMDISK} netbsd-INSTALL 
MDSET_RELEASEDIR=	installation/kernel

MDSET_NOSTRIP=		# defined

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
