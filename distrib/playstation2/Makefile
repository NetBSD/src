#	$NetBSD: Makefile,v 1.9 2002/04/21 11:20:45 shin Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

MINIROOTDIR!=	cd ${.CURDIR}/../miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTDIR}/miniroot.fs
MDSETTARGETS=	RAMDISK ${MINIROOT} netbsd

MDSETDIR=	installation

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
