#	$NetBSD: Makefile,v 1.3.2.2 1999/12/27 18:28:47 wrstuden Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=		sysinst.fs
KERNCNF=	${.CURDIR}/../../../../sys/arch/x68k/conf/INSTALL
KERNDIR=	${.CURDIR}/../../../../sys/arch/x68k/compile/INSTALL
KERN=		${KERNDIR}/netbsd
KERNNAME=	netbsdsi
RAMDISKDIR=	$(.CURDIR)/../ramdisk.sysinst/

.include "${TOP}/bootfloppy-common/Makefile.inc"
