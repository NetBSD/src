#	$NetBSD: Makefile,v 1.18 2002/10/17 02:05:36 lukem Exp $

BOOTMODEL=	big
IMAGE=		ramdisk-${BOOTMODEL}.fs
IMAGESIZE=	2048k
IMAGEDEPENDS= 	disktab.preinstall \
		${NETBSDSRCDIR}/usr.sbin/dhcp/clientscript/dhclient-script \
		${DESTDIR}/usr/mdec/biosboot_com0.sym
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"
