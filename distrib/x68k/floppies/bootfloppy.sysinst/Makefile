#	$NetBSD: Makefile,v 1.3 1999/12/25 14:30:25 minoura Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=		sysinst.fs
KERNOBJDIR?=	${.CURDIR}/../../../../sys/arch/x68k/compile
KERNDIR=	${KERNOBJDIR}/INSTALL
KERN=		${KERNDIR}/netbsd
KERNNAME=	netbsdsi
RAMDISKDIR=	$(.CURDIR)/../ramdisk.sysinst/

.include "${TOP}/bootfloppy-common/Makefile.inc"
