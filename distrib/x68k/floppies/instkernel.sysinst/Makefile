#	$NetBSD: Makefile,v 1.2 2002/05/07 14:08:33 isaki Exp $

TOP=		${.CURDIR}/..

.include <bsd.kernobj.mk>
.include <bsd.own.mk>

KERNDIR=	${KERNOBJDIR}/INSTALL
KERN=		${KERNDIR}/netbsd
KERNNAME=	netbsd.INSTALL
RAMDISKDIR=	${TOP}/ramdisk.sysinst/

.include "${TOP}/common/Makefile.instkernel"
