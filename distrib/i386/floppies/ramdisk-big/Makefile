#	$NetBSD: Makefile,v 1.15 2002/02/05 11:15:04 lukem Exp $

BOOTMODEL=	big
IMAGE=		ramdisk-${BOOTMODEL}.fs
IMAGESIZE=	2048k
IMAGEDEPENDS= 	disktab.preinstall \
		${_SRC_TOP_}/usr.sbin/dhcp/clientscript/dhclient-script \
		${DESTDIR}/usr/mdec/biosboot_com0.sym

.include "../common/Makefile.ramdisk"
