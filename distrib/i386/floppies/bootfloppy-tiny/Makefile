#	$NetBSD: Makefile,v 1.1 1998/10/12 16:32:17 perry Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-tiny.fs
KERN=	${.CURDIR}/../../../../sys/arch/i386/compile/INSTALL_TINY/netbsd
DISKTYPE=	floppy5
DISKSIZE=	2400

.include "${TOP}/bootfloppy-common/Makefile.inc"
