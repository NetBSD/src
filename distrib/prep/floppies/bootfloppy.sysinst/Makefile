#	$NetBSD: Makefile,v 1.3 2002/11/18 12:39:53 lukem Exp $

IMAGE=		sysinst.fs
BOOTLOADER=	${MDEC}/boot

.include <bsd.own.mk>
KERNDIR!=	cd ${.CURDIR}/../kernel-ramdisk && ${PRINTOBJDIR}
KERNEL=		${KERNDIR}/netbsd-INSTALL.gz

.include "../bootfloppy-common/Makefile.inc"
