#	$NetBSD: Makefile,v 1.17 2002/04/26 15:50:33 lukem Exp $

BOOTMODEL=	big
IMAGE=		ramdisk-${BOOTMODEL}.fs
IMAGESIZE=	2048k
IMAGEDEPENDS= 	disktab.preinstall \
		${NETBSDSRCDIR}/usr.sbin/dhcp/clientscript/dhclient-script \
		${DESTDIR}/usr/mdec/biosboot_com0.sym
USE_SYSINST=	yes

.include "${.CURDIR}/../common/Makefile.ramdisk"
