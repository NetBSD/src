#	$NetBSD: Makefile,v 1.1 1997/11/05 19:46:37 perry Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot${REV}-small.fs
KERN=	${.CURDIR}/../../../../sys/arch/i386/compile/INSTALL_SMALL/netbsd
DISKTYPE=	floppy5
DISKSIZE=	2400

.include "${TOP}/bootfloppy-common/Makefile.inc"
