#	$NetBSD: Makefile,v 1.1 2001/01/16 16:54:00 nonaka Exp $

IMAGE=		generic_com0.fs
BOOTLOADER=	${MDEC}/boot_com0

.include <bsd.own.mk>
KERNDIR!=	cd ${.CURDIR}/../kernel-generic && ${PRINTOBJDIR}
KERN=		${KERNDIR}/netbsd.GENERIC.gz

.include "../bootfloppy-common/Makefile.inc"
