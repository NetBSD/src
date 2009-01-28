#	$NetBSD: Makefile,v 1.3 2009/01/28 11:43:28 nonaka Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL ${RAMDISK} netbsd-INSTALL 
MDSET_RELEASEDIR=	installation/kernel

MDSET_NOGZIP=		# defined
MDSET_NOSTRIP=		# defined
MDSET_NOSYMBOLS=	# defined

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
