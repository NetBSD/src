#	$NetBSD: Makefile,v 1.19 2003/01/20 01:39:04 lukem Exp $

BOOTMODEL=	big
IMAGE=		ramdisk-${BOOTMODEL}.fs
IMAGESIZE=	2240k
IMAGEDEPENDS= 	disktab.preinstall \
		${NETBSDSRCDIR}/usr.sbin/dhcp/clientscript/dhclient-script \
		${DESTDIR}/usr/mdec/biosboot_com0.sym
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"

MAKEFS_FLAGS+=	-f 20
