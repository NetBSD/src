#	$NetBSD: Makefile,v 1.10 2002/04/26 15:50:43 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

MINIROOTDIR!=	cd ${.CURDIR}/../miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTDIR}/miniroot.fs
MDSETTARGETS=	RAMDISK ${MINIROOT} netbsd

MDSETDIR=	installation

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
