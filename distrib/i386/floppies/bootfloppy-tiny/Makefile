#	$NetBSD: Makefile,v 1.2 1999/03/23 18:10:59 drochner Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-tiny.fs
KERN?=	${.CURDIR}/../../../../sys/arch/i386/compile/INSTALL_TINY/netbsd
DISKTYPE=	floppy5
DISKSIZE=	2400

.include "${TOP}/bootfloppy-common/Makefile.inc"
