#	$NetBSD: Makefile,v 1.5 2002/03/05 20:29:35 thorpej Exp $

.include <bsd.own.mk>

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

BASENAME=boot-com
BOOTCODE=${MDEC}/biosboot_com0.sym	# Serial output

KERNDIR!=	cd ${TOP}/kernel-ramdisk && ${PRINTOBJDIR}
KERN=		${KERNDIR}/netbsd.INSTALL.gz

.include "${TOP}/common/Makefile.fdset"
