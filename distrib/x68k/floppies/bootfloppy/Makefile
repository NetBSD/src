#	$NetBSD: Makefile,v 1.5 1999/12/05 15:45:11 minoura Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=		boot.fs
KERNCNF=	${.CURDIR}/../../../../sys/arch/x68k/conf/INSTALL
KERNDIR=	${.CURDIR}/../../../../sys/arch/x68k/compile/INSTALL
KERN=		${KERNDIR}/netbsd
KERNNAME=	netbsd

.include "${TOP}/bootfloppy-common/Makefile.inc"
