#	$NetBSD: Makefile,v 1.12 2002/05/08 21:44:58 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

MINIROOTDIR!=	cd ${.CURDIR}/../miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTDIR}/miniroot.fs

MDSETTARGETS=		RAMDISK ${MINIROOT} netbsd
MDSET_RELEASEDIR=	installation

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
