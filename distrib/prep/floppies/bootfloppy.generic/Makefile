#	$NetBSD: Makefile,v 1.2 2002/04/12 14:52:58 lukem Exp $

IMAGE=		generic.fs
BOOTLOADER=	${MDEC}/boot

.include <bsd.own.mk>
KERNDIR!=	cd ${.CURDIR}/../kernel-generic && ${PRINTOBJDIR}
KERNEL=		${KERNDIR}/netbsd.GENERIC.gz

.include "../bootfloppy-common/Makefile.inc"
