#	$NetBSD: Makefile,v 1.3 2000/05/02 16:06:49 sjg Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-tiny.fs
KERN?=	${KERNOBJDIR}/INSTALL_TINY/netbsd
DISKTYPE=	floppy5
DISKSIZE=	2400

.include "${TOP}/bootfloppy-common/Makefile.inc"
