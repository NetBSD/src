#	$NetBSD: Makefile,v 1.4 2000/05/02 16:06:57 sjg Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=		sysinst.fs
KERNDIR=	${KERNOBJDIR}/INSTALL
KERN=		${KERNDIR}/netbsd
KERNNAME=	netbsdsi
RAMDISKDIR=	$(.CURDIR)/../ramdisk.sysinst/

.include "${TOP}/bootfloppy-common/Makefile.inc"
