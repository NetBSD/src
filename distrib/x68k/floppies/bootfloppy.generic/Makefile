#	$NetBSD: Makefile,v 1.1.2.2 2000/12/15 05:33:07 he Exp $

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
