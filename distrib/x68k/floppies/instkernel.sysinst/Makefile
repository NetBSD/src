#	$NetBSD: Makefile,v 1.1 2001/12/07 06:57:50 minoura Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

KERNDIR=	${KERNOBJDIR}/INSTALL
KERN=		${KERNDIR}/netbsd
KERNNAME=	netbsd.INSTALL
RAMDISKDIR=	$(.CURDIR)/../ramdisk.sysinst/

.include "${TOP}/instkernel-common/Makefile.inc"
