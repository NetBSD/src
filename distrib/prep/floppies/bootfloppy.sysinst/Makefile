#	$NetBSD: Makefile,v 1.2 2002/04/12 14:52:59 lukem Exp $

IMAGE=		sysinst.fs
BOOTLOADER=	${MDEC}/boot

.include <bsd.own.mk>
KERNDIR!=	cd ${.CURDIR}/../kernel-ramdisk && ${PRINTOBJDIR}
KERNEL=		${KERNDIR}/netbsd.INSTALL.gz

.include "../bootfloppy-common/Makefile.inc"
