#	$NetBSD: Makefile,v 1.6 2002/03/05 20:29:36 thorpej Exp $

.include <bsd.own.mk>

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	rescue-tiny.fs
KDIR=	${TOP}/kernel-rescue
KNAME=	INSTALL_TINY

KERNDIR!=	cd ${KDIR} && ${PRINTOBJDIR}
KERN=		${KERNDIR}/netbsd.${KNAME}.gz

DISKSIZE=	2400

.include "${TOP}/common/Makefile.bootfloppy"

${KERN}:
	cd ${KDIR} && ${MAKE}
