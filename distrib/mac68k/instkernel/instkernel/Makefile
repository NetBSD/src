#	$NetBSD: Makefile,v 1.17 2002/05/10 02:33:17 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL		${RAMDISK}	netbsd-INSTALL	\
			INSTALLSBC	${RAMDISK}	netbsd-INSTALLSBC
MDSET_RELEASEDIR=	installation/instkernel

MDSET_NOSTRIP.netbsd-INSTALL=		# defined
MDSET_NOSTRIP.netbsd-INSTALLSBC=	# defined

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
