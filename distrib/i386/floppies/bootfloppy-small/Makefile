#	$NetBSD: Makefile,v 1.4 2000/05/02 16:06:49 sjg Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-small.fs
KERN?=	${KERNOBJDIR}/INSTALL_SMALL/netbsd
DISKTYPE=	floppy5
DISKSIZE=	2400

.include "${TOP}/bootfloppy-common/Makefile.inc"
