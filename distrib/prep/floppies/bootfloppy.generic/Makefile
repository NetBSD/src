#	$NetBSD: Makefile,v 1.3 2002/11/18 12:39:53 lukem Exp $

IMAGE=		generic.fs
BOOTLOADER=	${MDEC}/boot

.include <bsd.own.mk>
KERNDIR!=	cd ${.CURDIR}/../kernel-generic && ${PRINTOBJDIR}
KERNEL=		${KERNDIR}/netbsd-GENERIC.gz

.include "../bootfloppy-common/Makefile.inc"
