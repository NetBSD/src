#	$NetBSD: Makefile,v 1.5 2002/03/05 20:29:34 thorpej Exp $

.include <bsd.own.mk>

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-ps2.fs

KERNDIR!=	cd ${TOP}/kernel-ramdisk && ${PRINTOBJDIR}
KERN=		${KERNDIR}/netbsd.INSTALL_PS2.gz

DISKSIZE=	2880

.include "${TOP}/common/Makefile.bootfloppy"
