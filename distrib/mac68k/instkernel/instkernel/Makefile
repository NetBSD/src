#	$NetBSD: Makefile,v 1.18 2003/10/25 04:39:48 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL		${RAMDISK}	netbsd-INSTALL	\
			INSTALLSBC	${RAMDISK}	netbsd-INSTALLSBC
MDSET_RELEASEDIR=	installation/instkernel

MDSET_NOSTRIP=		# defined

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
