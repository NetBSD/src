#	$NetBSD: Makefile,v 1.6 2002/03/05 20:29:35 thorpej Exp $

.include <bsd.own.mk>

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

BASENAME=bootlap

KERNDIR!=	cd ${TOP}/kernel-ramdisk && ${PRINTOBJDIR}
KERN=		${KERNDIR}/netbsd.INSTALL_LAPTOP.gz

.include "${TOP}/common/Makefile.fdset"
