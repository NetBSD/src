#	$NetBSD: Makefile,v 1.16 2002/02/08 05:02:17 lukem Exp $

BOOTMODEL=	big
IMAGE=		ramdisk-${BOOTMODEL}.fs
IMAGESIZE=	2048k
IMAGEDEPENDS= 	disktab.preinstall \
		${_SRC_TOP_}/usr.sbin/dhcp/clientscript/dhclient-script \
		${DESTDIR}/usr/mdec/biosboot_com0.sym
USE_SYSINST=	yes

.include "${.CURDIR}/../common/Makefile.ramdisk"
