#	$NetBSD: Makefile,v 1.2 1997/11/29 21:19:03 perry Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-small.fs
KERN=	${.CURDIR}/../../../../sys/arch/i386/compile/INSTALL_SMALL/netbsd
DISKTYPE=	floppy5
DISKSIZE=	2400

.include "${TOP}/bootfloppy-common/Makefile.inc"
