#	$NetBSD: Makefile,v 1.7 1999/12/25 14:30:24 minoura Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=		boot.fs
KERNOBJDIR?=	${.CURDIR}/../../../../sys/arch/x68k/compile
KERNDIR=	${KERNOBJDIR}/INSTALL_SMALL
KERN=		${KERNDIR}/netbsd
KERNNAME=	netbsd

.include "${TOP}/bootfloppy-common/Makefile.inc"
