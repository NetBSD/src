#	$NetBSD: Makefile,v 1.9 2002/03/05 20:29:35 thorpej Exp $

.include <bsd.own.mk>

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-tiny.fs

KERNDIR!=	cd ${TOP}/kernel-ramdisk && ${PRINTOBJDIR}
KERN=		${KERNDIR}/netbsd.INSTALL_TINY.gz

DISKSIZE=	2400

.include "${TOP}/common/Makefile.bootfloppy"
