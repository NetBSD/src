#	$NetBSD: Makefile,v 1.4 1999/07/06 16:22:58 minoura Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=		boot.fs
KERNCNF=	${.CURDIR}/../../../../sys/arch/x68k/conf/INSTALL
KERNDIR=	${.CURDIR}/../../../../sys/arch/x68k/compile/INSTALL
KERN=		${KERNDIR}/netbsd
KERNNAME=	netbsd.INSTALL

.include "${TOP}/bootfloppy-common/Makefile.inc"
