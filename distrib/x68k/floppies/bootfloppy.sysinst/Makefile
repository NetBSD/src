#	$NetBSD: Makefile,v 1.6 2000/11/29 08:28:55 minoura Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

IMAGE=		sysinst.fs
KERNDIR=	${KERNOBJDIR}/INSTALL
KERN=		${KERNDIR}/netbsd
KERNNAME=	netbsd
RAMDISKDIR=	$(.CURDIR)/../ramdisk.sysinst/

.include "${TOP}/bootfloppy-common/Makefile.inc"
