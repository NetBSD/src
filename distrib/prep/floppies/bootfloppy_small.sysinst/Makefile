#	$NetBSD: Makefile,v 1.1 2006/06/20 17:40:36 garbled Exp $

IMAGE=		sysinst_small.fs
BOOTLOADER=	${MDEC}/boot

.include <bsd.own.mk>
KERNDIR!=	cd ${.CURDIR}/../kernel-ramdisk && ${PRINTOBJDIR}
KERNEL=		${KERNDIR}/netbsd-INSTALL_SMALL.gz

.include "../bootfloppy-common/Makefile.inc"
