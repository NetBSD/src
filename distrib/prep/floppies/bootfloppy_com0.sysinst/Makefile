#	$NetBSD: Makefile,v 1.2 2002/04/12 14:53:00 lukem Exp $

IMAGE=		sysinst_com0.fs
BOOTLOADER=	${MDEC}/boot_com0

.include <bsd.own.mk>
KERNDIR!=	cd ${.CURDIR}/../kernel-ramdisk && ${PRINTOBJDIR}
KERNEL=		${KERNDIR}/netbsd.INSTALL.gz

.include "../bootfloppy-common/Makefile.inc"
