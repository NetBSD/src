#	$NetBSD: Makefile,v 1.1 2001/01/16 16:53:58 nonaka Exp $

IMAGE=		generic.fs
BOOTLOADER=	${MDEC}/boot

.include <bsd.own.mk>
KERNDIR!=	cd ${.CURDIR}/../kernel-generic && ${PRINTOBJDIR}
KERN=		${KERNDIR}/netbsd.GENERIC.gz

.include "../bootfloppy-common/Makefile.inc"
