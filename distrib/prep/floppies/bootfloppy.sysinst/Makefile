#	$NetBSD: Makefile,v 1.1 2001/01/16 16:54:00 nonaka Exp $

IMAGE=		sysinst.fs
BOOTLOADER=	${MDEC}/boot

.include <bsd.own.mk>
KERNDIR!=	cd ${.CURDIR}/../kernel-ramdisk && ${PRINTOBJDIR}
KERN=		${KERNDIR}/netbsd.INSTALL.gz

.include "../bootfloppy-common/Makefile.inc"
