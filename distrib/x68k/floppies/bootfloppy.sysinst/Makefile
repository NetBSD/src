#	$NetBSD: Makefile,v 1.7 2001/12/07 06:57:50 minoura Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

BASENAME=	sysinst
IMAGE1=		${BASENAME}.fs
INSTKERNDIR=	${TOP}/instkernel.sysinst
INSTKERNNAME=	netbsd.INSTALL.gz

.include "${TOP}/bootfloppy-common/Makefile.inc"
