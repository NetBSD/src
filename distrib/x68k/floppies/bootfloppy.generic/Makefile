#	$NetBSD: Makefile,v 1.1 2000/11/28 03:03:27 minoura Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

IMAGE=		kern.fs
KERNDIR=	${KERNOBJDIR}/GENERIC
KERN=		${KERNDIR}/netbsd
RAMDISKDIR=
NOKERNCOPY=	YES

.include "${TOP}/bootfloppy-common/Makefile.inc"
