#	$NetBSD: Makefile,v 1.3 2002/11/18 12:39:54 lukem Exp $

IMAGE=		sysinst_com0.fs
BOOTLOADER=	${MDEC}/boot_com0

.include <bsd.own.mk>
KERNDIR!=	cd ${.CURDIR}/../kernel-ramdisk && ${PRINTOBJDIR}
KERNEL=		${KERNDIR}/netbsd-INSTALL.gz

.include "../bootfloppy-common/Makefile.inc"
