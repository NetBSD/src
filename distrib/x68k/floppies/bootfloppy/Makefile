#	$NetBSD: Makefile,v 1.4.4.1 1999/12/27 18:28:47 wrstuden Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=		boot.fs
KERNCNF=	${.CURDIR}/../../../../sys/arch/x68k/conf/INSTALL
KERNDIR=	${.CURDIR}/../../../../sys/arch/x68k/compile/INSTALL
KERN=		${KERNDIR}/netbsd
KERNNAME=	netbsd

.include "${TOP}/bootfloppy-common/Makefile.inc"
