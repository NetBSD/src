#	$NetBSD: Makefile,v 1.9 2000/06/16 16:07:09 minoura Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

IMAGE=		boot.fs
KERNDIR=	${KERNOBJDIR}/INSTALL_SMALL
KERN=		${KERNDIR}/netbsd
KERNNAME=	netbsd

.include "${TOP}/bootfloppy-common/Makefile.inc"
